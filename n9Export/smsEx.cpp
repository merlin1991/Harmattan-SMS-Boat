#include <iostream>

#include <QtCore>
#include <QDebug>
#include <CommHistory/SyncSMSModel>
#include <CommHistory/Event>

using namespace CommHistory;

int main(int argc, char** argv) 
{
    QCoreApplication app(argc, argv);

    SyncSMSModel smsModel(ALL);

    smsModel.enableContactChanges(false);
    smsModel.setQueryMode(EventModel::SyncQuery);
    if(!smsModel.getEvents())
    {
        qCritical() << "Error fetching events";
        return -1;
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
            std::string direction = e.direction() == Event::Inbound ? "IN" : "OUT";
            std::cout << qPrintable(e.remoteUid()) << ";" <<
                direction << ";" << 
                qPrintable(e.startTime().toString(Qt::ISODate)) << ";" <<
                qPrintable(e.endTime().toString(Qt::ISODate)) << ";" <<
                qPrintable(e.freeText().replace('\n', "\n ")) << std::endl;
        }
    }

    return 0;
}
