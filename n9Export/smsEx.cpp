#include <QtCore>
#include <QDebug>
#include <CommHistory/SyncSMSModel>
#include <CommHistory/Event>

using namespace CommHistory;

int main(int argc, char** argv) 
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    if(args.size() != 2)
    {
        qCritical() << "no filename to write into was given";
        return EXIT_FAILURE;
    }

    QFile outputFile(args.at(1));

    if(!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qCritical() << "could not open" << args.at(1) << "for writing";
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
