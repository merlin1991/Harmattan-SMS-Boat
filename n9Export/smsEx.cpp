#include <argp.h>

#include <QtCore>
#include <QDebug>
#include <CommHistory/SyncSMSModel>
#include <CommHistory/Event>

using namespace CommHistory;

struct RuntimeSettings {
    QString file;
};

static char doc[] =
    "smsExport -- exports all sms via libcommhistory to a csv like FILE";

static char args_doc[] = "FILE";

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp_option options[] = {
    { 0 }
};

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
    struct RuntimeSettings *conf = (struct RuntimeSettings*)state->input;
   
    switch(key)
    {
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

int main(int argc, char** argv) 
{
    struct RuntimeSettings conf;

    if(argp_parse(&arg_parser, argc, argv, 0, 0, &conf))
    {
        qCritical() << "argument parsing error";
        return EXIT_FAILURE;
    }

    QCoreApplication app(argc, argv);
    QFile outputFile(conf.file);

    if(!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical() << "could not open" << conf.file << "for writing";
        return EXIT_FAILURE;
    }

    QTextStream outputStream(&outputFile);
    outputStream.setCodec("utf-8");

    SyncSMSModel smsModel(ALL);

    smsModel.enableContactChanges(false);
    smsModel.setQueryMode(EventModel::SyncQuery);
    if(!smsModel.getEvents())
    {
        qCritical() << "Error fetching events";
        return EXIT_FAILURE;
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
            outputStream << e.remoteUid() << ";" <<
                direction << ";" << 
                e.startTime().toString(Qt::ISODate) << ";" <<
                e.endTime().toString(Qt::ISODate) << ";" <<
                e.freeText().replace('\n', "\n ") << endl;
        }
    }

    outputStream.flush();
    outputFile.close();

    return EXIT_SUCCESS;
}
