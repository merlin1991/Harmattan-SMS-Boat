#include <iostream>

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

static GroupModel groupModel;
Catcher groupCatcher(&groupModel);
static EventModel eventModel;
Catcher eventCatcher(&eventModel);

void addContact(QHash<QString, int>* list, QString number)
{
    Group group;
    group.setLocalUid(RING_ACCOUNT);
    group.setRemoteUids(QStringList() << number);
    group.setChatType(Group::ChatTypeP2P);
    groupCatcher.reset();
    if(!groupModel.addGroup(group))
    {
        qWarning() << "could not add group for" << number;
        return;
    }

    groupCatcher.waitCommit();
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

    eventCatcher.reset();
    if(!eventModel.addEvent(event))
    {
        qWarning() << "could not add message " << message;
        return;
    }

    eventCatcher.waitCommit();
    qDebug() << "message from/for" << tokens.at(0) << "added";
}

int main(int argc, char** argv) 
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    if(args.size() != 2)
    {
        qCritical() << "No file given";
        return EXIT_FAILURE;
    }

    QFile csvFile(args.at(1));
    if(!csvFile.exists())
    {
        qCritical() << args.at(1) << "does not exist";
        return EXIT_FAILURE;
    }

    if(!csvFile.open(QFile::ReadOnly))
    {
        qCritical() << args.at(1) << "could not be opened";
        return EXIT_FAILURE;
    }

    QTextStream csvStream(&csvFile);
    QString lineBuffer;
    QString csvLine;

    groupModel.enableContactChanges(false);
    groupModel.setQueryMode(EventModel::SyncQuery);

    while(!csvStream.atEnd())
    {
        csvLine = csvStream.readLine();
        if(csvLine.startsWith(' '))
        {
            lineBuffer = lineBuffer % "\n" % csvLine.mid(1);
            continue;
        }
        workMessage(&lineBuffer);
        lineBuffer = csvLine;
    }
    workMessage(&lineBuffer);
    
    return EXIT_SUCCESS;
}
