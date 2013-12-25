#include <argp.h>

#include <QtCore>
#include <QString>
#include <QHash>
#include <QDebug>
#include <QStringList>
#include <QFile>
#include <QTextStream>

#include <CommHistory/GroupModel>
#include <CommHistory/EventModel>
#include <CommHistory/Group>
#include <CommHistory/Event>

#include "catcher.h"

#define RING_ACCOUNT "/org/freedesktop/Telepathy/Account/ring/tel/account0"

using namespace CommHistory;

static GroupModel* groupModel;
static Catcher* groupCatcher;
static EventModel* eventModel;
static Catcher* eventCatcher;

enum ImportMode {
    SMS,
    CALLS
};

struct RuntimeSettings {
    QString file;
    enum ImportMode mode;
};

static char doc[] =
    "jollaImport -- imports sms/calls via libcommhistory from a csv like FILE";

static char args_doc[] = "FILE";

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp_option options[] = {
    { "calls", 'c', 0, 0, "import calls"},
    { "sms", 's', 0, 0, "import sms (default)" },
    { 0 }
};
#pragma GCC diagnostic pop

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
    struct RuntimeSettings *conf = (struct RuntimeSettings*)state->input;
   
    switch(key)
    {
        case 'c':
            conf->mode = CALLS;
            break;
        case 's':
            conf->mode = SMS;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                /* Too many arguments. */
                argp_usage (state);
            conf->file = QString::fromLocal8Bit(arg);
            break;
     
         case ARGP_KEY_END:
            if (state->arg_num < 1)
                /* Not enough arguments. */
                argp_usage (state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp arg_parser = {options, parse_opt, args_doc, doc};
#pragma GCC diagnostic pop

void addContact(QHash<QString, int>* list, QString number)
{
    Group group;
    group.setLocalUid(RING_ACCOUNT);
    group.setRemoteUids(QStringList() << number);
    group.setChatType(Group::ChatTypeP2P);
    groupCatcher->reset();
    if(!groupModel->addGroup(group))
    {
        qWarning() << "could not add group for" << number;
        return;
    }

    groupCatcher->waitCommit();
    list->insert(number, group.id());
}

void workMessage(QString* message)
{
    static QHash<QString, int> contactList;

    QStringList tokens = message->split(';');
    if(tokens.size() < 5)
    {
        qDebug() << "invalid message:" << qPrintable(*message);
        return;
    }
    if(!contactList.contains(tokens.at(0)))
        addContact(&contactList, tokens.at(0));
    
    Event event;
    event.setType(Event::SMSEvent);
    event.setGroupId(contactList.value(tokens.at(0)));
    event.setLocalUid(RING_ACCOUNT);
    event.setRemoteUid(tokens.at(0));
    event.setDirection(tokens.at(1) == "IN" ? Event::Inbound : Event::Outbound);
    event.setStatus(Event::DeliveredStatus);

    QDateTime date = QDateTime::fromString(tokens.at(2), Qt::ISODate);
    QDateTime endDate = QDateTime::fromString(tokens.at(3), Qt::ISODate);
    event.setStartTime(date);
    event.setEndTime(endDate);
    event.setIsRead(true);
    event.setFreeText(message->section(';', 4));

    eventCatcher->reset();
    if(!eventModel->addEvent(event))
    {
        qWarning() << "could not add message " << message;
        return;
    }

    eventCatcher->waitCommit();
    qDebug() << "message from/for" << tokens.at(0) << "added";
}

void workCall(QString *call)
{
    QStringList tokens = call->split(';');
    if(tokens.size() < 5)
    {
        qDebug() << "invalid call:" << qPrintable(*call);
        return;
    }
    Event event;
    event.setType(Event::CallEvent);
    event.setGroupId(-1);
    event.setLocalUid(RING_ACCOUNT);
    event.setRemoteUid(tokens.at(0));
    event.setDirection(tokens.at(1) == "IN" ? Event::Inbound : Event::Outbound);
    event.setStatus(Event::UnknownStatus);
    event.setIsMissedCall(tokens.at(2) == "OK" ? false : true);

    QDateTime date = QDateTime::fromString(tokens.at(3), Qt::ISODate);
    QDateTime endDate = QDateTime::fromString(tokens.at(4), Qt::ISODate);
    event.setStartTime(date);
    event.setEndTime(endDate);
    
    eventCatcher->reset();
    if(!eventModel->addEvent(event))
    {
        qWarning() << "could not add call " << call;
        return;
    }

    eventCatcher->waitCommit();
    qDebug() << "call from/to" << tokens.at(0) << "added";
}

int runImport(struct RuntimeSettings *conf)
{
    QFile csvFile(conf->file);
    if(!csvFile.exists())
    {
        qCritical() << conf->file << "does not exist";
        return 0;
    }

    if(!csvFile.open(QFile::ReadOnly))
    {
        qCritical() << conf->file << "could not be opened";
        return 0;
    }

    QTextStream csvStream(&csvFile);
    csvStream.setCodec("utf-8");
    QString lineBuffer;
    QString csvLine;

    if(conf->mode == SMS)
    {
        groupModel = new GroupModel();
        groupCatcher = new Catcher(groupModel);
        groupModel->enableContactChanges(false);
        groupModel->setQueryMode(EventModel::SyncQuery);
    }

    eventModel = new EventModel();
    eventCatcher = new Catcher(eventModel);

    while(!csvStream.atEnd())
    {
        csvLine = csvStream.readLine();
        if(conf->mode == CALLS)
        {
            workCall(&csvLine);
            continue;
        }
        if(csvLine.startsWith(' '))
        {
            lineBuffer = lineBuffer % "\n" % csvLine.mid(1);
            continue;
        }
        workMessage(&lineBuffer);
        lineBuffer = csvLine;
    }

    if(conf->mode == SMS)
    {
        workMessage(&lineBuffer);

        delete groupCatcher;
        delete groupModel;
    }

    delete eventCatcher;
    delete eventModel;
    
    return 1;
}

int main(int argc, char** argv) 
{
    struct RuntimeSettings conf;
    conf.mode = SMS;

    if(argp_parse(&arg_parser, argc, argv, 0, 0, &conf))
    {
        qCritical() << "argument parsing error";
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);

    switch(conf.mode)
    {
        case SMS:
        case CALLS:
            if(runImport(&conf))
            {
                return EXIT_SUCCESS;
            }
        default:
            qCritical() << "invalid import mode";
            break;
    }

    return EXIT_FAILURE;
}
