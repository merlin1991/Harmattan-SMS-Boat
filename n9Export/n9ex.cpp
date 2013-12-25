#include <argp.h>

#include <QtCore>
#include <QDebug>
#include <CommHistory/CallModel>
#include <CommHistory/SyncSMSModel>
#include <CommHistory/Event>

using namespace CommHistory;

enum ExportMode {
    SMS,
    CALLS
};

struct RuntimeSettings {
    QString file;
    enum ExportMode mode;
};

static char doc[] =
    "n9export -- exports all sms/calls via libcommhistory to a csv like FILE";

static char args_doc[] = "FILE";

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp_option options[] = {
    { "calls", 'c', 0, 0, "export calls"},
    { "sms", 's', 0, 0, "export sms (default)" },
    { 0 }
};

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

static struct argp arg_parser = {options, parse_opt, args_doc, doc};

int exportCALLS(QTextStream *outputStream)
{
    CallModel callModel;

    callModel.enableContactChanges(false);
    callModel.setQueryMode(EventModel::SyncQuery);
    callModel.setTreeMode(false);
    callModel.setFilter(CallModel::SortByTime);
    if(!callModel.getEvents())
    {
        qCritical() << "Error fetching calls";
        return 0;
    }

    int i;

    qDebug() << "# calls: " << callModel.rowCount();

    for(i = callModel.rowCount() - 1; i >= 0; i--) //we iterate backwards, because the jolla does not like to import the last call first
    {
        Event e = callModel.event(callModel.index(i, 0));
        if(e.type() != Event::CallEvent)
        {
            qDebug() << "Not a call";
        }
        else
        {

            QString direction = e.direction() == Event::Inbound ? "IN" : "OUT";
            QString missed = e.isMissedCall() ? "MISSED" : "OK";
            *outputStream << e.remoteUid() << ";" <<
                direction << ";" << 
                missed << ";" <<
                e.startTime().toString(Qt::ISODate) << ";" <<
                e.endTime().toString(Qt::ISODate) << endl;
        }
    }

    return 1;
}

int exportSMS(QTextStream *outputStream)
{
    SyncSMSModel smsModel(ALL);

    smsModel.enableContactChanges(false);
    smsModel.setQueryMode(EventModel::SyncQuery);
    if(!smsModel.getEvents())
    {
        qCritical() << "Error fetching sms";
        return 0;
    }

    int i;

    qDebug() << "# sms: " << smsModel.rowCount();

    for(i = 0; i < smsModel.rowCount(); i++)
    {
        Event e = smsModel.event(smsModel.index(i, 0));
        if(e.type() != Event::SMSEvent)
        {
            qDebug() << "Not an SMS";
        }
        else
        {

            QString direction = e.direction() == Event::Inbound ? "IN" : "OUT";
            *outputStream << e.remoteUid() << ";" <<
                direction << ";" << 
                e.startTime().toString(Qt::ISODate) << ";" <<
                e.endTime().toString(Qt::ISODate) << ";" <<
                e.freeText().replace('\n', "\n ") << endl;
        }
    }

    return 1;
}

int runExport(struct RuntimeSettings *conf)
{
    QFile outputFile(conf->file);

    if(!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical() << "could not open" << conf->file << "for writing";
        return 0;
    }

    QTextStream outputStream(&outputFile);
    outputStream.setCodec("utf-8");

    int returnCode;
    switch(conf->mode)
    {
        case SMS:
            returnCode = exportSMS(&outputStream);
            break;
        case CALLS:
            returnCode = exportCALLS(&outputStream);
            break;
    }

    outputStream.flush();
    outputFile.close();

    return returnCode;
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
            if(runExport(&conf))
            {
                return EXIT_SUCCESS;
            }
            break;
        default:
            qCritical() << "invalid export mode";
            break;
    }
    return EXIT_FAILURE;
}
