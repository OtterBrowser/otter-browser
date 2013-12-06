#include "TransfersManager.h"

namespace Otter
{

TransfersManager* TransfersManager::m_instance = NULL;
QList<TransferInformation*> TransfersManager::m_transfers;

TransfersManager::TransfersManager(QObject *parent) : QObject(parent)
{
}

void TransfersManager::createInstance(QObject *parent)
{
	m_instance = new TransfersManager(parent);
}

TransfersManager *TransfersManager::getInstance()
{
	return m_instance;
}

TransferInformation *TransfersManager::startTransfer(const QString &source, const QString &target)
{
//TODO
}

QList<TransferInformation *> TransfersManager::getTransfers()
{
	return m_transfers;
}

bool TransfersManager::restartTransfer(TransferInformation *transfer)
{
//TODO
}

bool TransfersManager::resumeTransfer(TransferInformation *transfer)
{
//TODO
}

bool TransfersManager::removeTransfer(TransferInformation *transfer, bool keepFile)
{
//TODO
}

bool TransfersManager::stopTransfer(TransferInformation *transfer)
{
//TODO
}

}
