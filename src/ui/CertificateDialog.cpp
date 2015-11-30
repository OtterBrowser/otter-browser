/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "CertificateDialog.h"

#include "ui_CertificateDialog.h"

#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtNetwork/QSslCertificateExtension>
#include <QtNetwork/QSslKey>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

CertificateDialog::CertificateDialog(QList<QSslCertificate> certificates, QWidget *parent) : Dialog(parent),
	m_certificates(certificates),
	m_ui(new Ui::CertificateDialog)
{
	m_ui->setupUi(this);
	m_ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Export…"));

	if (certificates.isEmpty())
	{
		setWindowTitle(tr("Invalid Certificate"));

		return;
	}

	setWindowTitle(tr("View Certificate for %1").arg(certificates.first().subjectInfo(QSslCertificate::CommonName).join(QLatin1String(", "))));

	QStandardItemModel *chainModel = new QStandardItemModel(this);
	QStandardItem *certificateItem = NULL;

	for (int i = (certificates.count() - 1); i >= 0; --i)
	{
		QStandardItem *parentCertificateItem = certificateItem;

		certificateItem = new QStandardItem(certificates.at(i).subjectInfo(QSslCertificate::Organization).value(0, tr("Unknown")));
		certificateItem->setData(i, Qt::UserRole);

		if (parentCertificateItem)
		{
			parentCertificateItem->appendRow(certificateItem);
		}
		else
		{
			chainModel->appendRow(certificateItem);
		}
	}

	m_ui->chainItemView->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->chainItemView->setModel(chainModel);
	m_ui->chainItemView->expandAll();
	m_ui->chainItemView->setCurrentIndex(chainModel->index(0, 0));
	m_ui->detailsItemView->setViewMode(ItemViewWidget::TreeViewMode);
	m_ui->detailsItemView->setModel(new QStandardItemModel(this));

	selectCertificate(chainModel->index(0, 0));

	connect(m_ui->chainItemView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectCertificate(QModelIndex)));
	connect(m_ui->detailsItemView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectField(QModelIndex)));
	connect(m_ui->buttonBox->button(QDialogButtonBox::Save), SIGNAL(clicked(bool)), this, SLOT(exportCertificate()));
}

CertificateDialog::~CertificateDialog()
{
	delete m_ui;
}

void CertificateDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void CertificateDialog::exportCertificate()
{
	const QSslCertificate certificate = m_certificates.value(m_ui->chainItemView->currentIndex().data(Qt::UserRole).toInt());

	if (certificate.isNull())
	{
		return;
	}

	QString filter;
	const QString fileName = QFileDialog::getSaveFileName(this, tr("Select File"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), tr("DER encoded X509 certificates (*.der);;PEM encoded X509 certificates (*.pem);;Text files (*.txt)"), &filter);

	if (!fileName.isEmpty())
	{
		QFile file(fileName);

		if (!file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(this, tr("Error"), tr("Filed to open file for writing."), QMessageBox::Close);

			return;
		}

		if (filter.contains(QLatin1String(".der")))
		{
			file.write(certificate.toDer());
		}
		else if (filter.contains(QLatin1String(".pem")))
		{
			file.write(certificate.toPem());
		}
		else
		{
			QTextStream stream(&file);
			stream << certificate.toText();
		}

		file.close();
	}
}

void CertificateDialog::selectCertificate(const QModelIndex &index)
{
	const QSslCertificate certificate = m_certificates.value(index.data(Qt::UserRole).toInt());

	m_ui->detailsItemView->getModel()->clear();

	createField(VersionField);
	createField(SerialNumberField);
	createField(SignatureAlgorithmField)->setEnabled(false);
	createField(IssuerField);

	QStandardItem *validityItem = createField(ValidityField);
	validityItem->setFlags(Qt::ItemIsEnabled);

	createField(ValidityNotBeforeField, validityItem);
	createField(ValidityNotAfterField, validityItem);
	createField(SubjectField);

	QStandardItem *publicKeyItem = createField(PublicKeyField);
	publicKeyItem->setFlags(Qt::ItemIsEnabled);
	publicKeyItem->setEnabled(false);

	createField(PublicKeyAlgorithmField, publicKeyItem)->setEnabled(false);
	createField(PublicKeyValueField, publicKeyItem)->setEnabled(false);

	QStandardItem *extensionsItem = createField(ExtensionsField);
	extensionsItem->setFlags(Qt::ItemIsEnabled);
	extensionsItem->setEnabled(certificate.extensions().count() > 0);

	for (int i = 0; i < certificate.extensions().count(); ++i)
	{
		QString title = certificate.extensions().at(i).name();

		if (title == QLatin1String("authorityKeyIdentifier"))
		{
			title = tr("Authority Key Identifier");
		}
		else if (title == QLatin1String("subjectKeyIdentifier"))
		{
			title = tr("Subject Key Identifier");
		}
		else if (title == QLatin1String("keyUsage"))
		{
			title = tr("Key Usage");
		}
		else if (title == QLatin1String("certificatePolicies"))
		{
			title = tr("Certificate Policies");
		}
		else if (title == QLatin1String("policyMappings"))
		{
			title = tr("Policy Mappings");
		}
		else if (title == QLatin1String("subjectAltName"))
		{
			title = tr("Subject Alternative Name");
		}
		else if (title == QLatin1String("issuerAltName"))
		{
			title = tr("Issuer Alternative Name");
		}
		else if (title == QLatin1String("subjectDirectoryAttributes"))
		{
			title = tr("Subject Directory Attributes");
		}
		else if (title == QLatin1String("basicConstraints"))
		{
			title = tr("Basic Constraints");
		}
		else if (title == QLatin1String("nameConstraints"))
		{
			title = tr("Name Constraints");
		}
		else if (title == QLatin1String("policyConstraints"))
		{
			title = tr("Policy Constraints");
		}
		else if (title == QLatin1String("extendedKeyUsage"))
		{
			title = tr("Extended Key Usage");
		}
		else if (title == QLatin1String("crlDistributionPoints"))
		{
			title = tr("CRL Distribution Points");
		}
		else if (title == QLatin1String("inhibitAnyPolicy"))
		{
			title = tr("Inhibit anyPolicy");
		}
		else if (title == QLatin1String("freshestCRL"))
		{
			title = tr("Delta CRL Distribution Point");
		}
		else if (title == QLatin1String("authorityInfoAccess"))
		{
			title = tr("Authority Information Access");
		}
		else if (title == QLatin1String("subjectInfoAccess"))
		{
			title = tr("Subject Information Access");
		}

		QMap<int, QVariant> data;
		data[Qt::DisplayRole] = title;
		data[Qt::UserRole + 1] = i;

		createField(ExtensionField, extensionsItem, data);
	}

	QStandardItem *digestItem = createField(DigestField);

	createField(DigestSha256Field, digestItem);
	createField(DigestSha1Field, digestItem);

	m_ui->detailsItemView->expandAll();
	m_ui->valueTextEdit->clear();
}

void CertificateDialog::selectField(const QModelIndex &index)
{
	const QSslCertificate certificate = m_certificates.value(m_ui->chainItemView->currentIndex().data(Qt::UserRole).toInt());
	const CertificateField field = static_cast<CertificateField>(index.data(Qt::UserRole).toInt());

	m_ui->valueTextEdit->clear();

	switch (field)
	{
		case ValidityField:
		case PublicKeyValueField:
		case ExtensionsField:
		case DigestField:
			break;
		case VersionField:
			m_ui->valueTextEdit->setPlainText(QString(certificate.version()));

			break;
		case SerialNumberField:
			m_ui->valueTextEdit->setPlainText(QString(certificate.serialNumber()).toUpper());

			break;
		case SignatureAlgorithmField:
///FIXME
			break;
		case IssuerField:
			{
				const QList<QByteArray> attributes = certificate.issuerInfoAttributes();

				for (int i = 0; i < attributes.count(); ++i)
				{
					m_ui->valueTextEdit->appendPlainText(QStringLiteral("%1 = %2").arg(QString(attributes.at(i))).arg(certificate.issuerInfo(attributes.at(i)).join(QLatin1String(", "))));
				}
			}

			break;
		case ValidityNotBeforeField:
			m_ui->valueTextEdit->setPlainText(certificate.effectiveDate().toString(QLatin1String("yyyy-MM-dd hh:mm:ss t")));

			break;
		case ValidityNotAfterField:
			m_ui->valueTextEdit->setPlainText(certificate.expiryDate().toString(QLatin1String("yyyy-MM-dd hh:mm:ss t")));

			break;
		case SubjectField:
			{
				const QList<QByteArray> attributes = certificate.subjectInfoAttributes();

				for (int i = 0; i < attributes.count(); ++i)
				{
					m_ui->valueTextEdit->appendPlainText(QStringLiteral("%1 = %2").arg(QString(attributes.at(i))).arg(certificate.subjectInfo(attributes.at(i)).join(QLatin1String(", "))));
				}
			}

			break;
		case PublicKeyField:
///FIXME
			break;
		case PublicKeyAlgorithmField:
///FIXME
			break;
		case ExtensionField:
			{
				const QSslCertificateExtension extension = certificate.extensions().value(index.data(Qt::UserRole + 1).toInt());

				m_ui->valueTextEdit->setPlainText(extension.isCritical() ? tr("Critical") : tr("Not Critical"));
				m_ui->valueTextEdit->appendPlainText(tr("OID: %1").arg(extension.oid()));

				if (!extension.value().isNull())
				{
					m_ui->valueTextEdit->appendPlainText(tr("Value:"));

					if (extension.value().type() == QVariant::List)
					{
						const QVariantList list = extension.value().toList();

						for (int i = 0; i < list.count(); ++i)
						{
							m_ui->valueTextEdit->appendPlainText(list.at(i).toString());
						}
					}
					else if (extension.value().type() == QVariant::Map)
					{
						const QVariantMap map = extension.value().toMap();
						QVariantMap::const_iterator iterator;

						for (iterator = map.constBegin(); iterator != map.constEnd(); ++iterator)
						{
							m_ui->valueTextEdit->appendPlainText(QStringLiteral("%1 = %2").arg(iterator.key()).arg(iterator.value().toString()));
						}
					}
					else
					{
						m_ui->valueTextEdit->appendPlainText(extension.value().toString());
					}
				}
			}

			break;
		case DigestSha1Field:
			m_ui->valueTextEdit->setPlainText(QString(certificate.digest(QCryptographicHash::Sha1).toHex()).toUpper());

			break;
		case DigestSha256Field:
			m_ui->valueTextEdit->setPlainText(QString(certificate.digest(QCryptographicHash::Sha256).toHex()).toUpper());

			break;
		default:
			break;
	}

	QTextCursor cursor(m_ui->valueTextEdit->textCursor());
	cursor.setPosition(0);

	m_ui->valueTextEdit->setTextCursor(cursor);
}

QStandardItem* CertificateDialog::createField(CertificateDialog::CertificateField field, QStandardItem *parent, const QMap<int, QVariant> &data)
{
	QString title;

	switch (field)
	{
		case VersionField:
			title = tr("Version");

			break;
		case SerialNumberField:
			title = tr("Serial Number");

			break;
		case SignatureAlgorithmField:
			title = tr("Certificate Signature Algorithm");

			break;
		case IssuerField:
			title = tr("Issuer");

			break;
		case ValidityField:
			title = tr("Validity");

			break;
		case ValidityNotBeforeField:
			title = tr("Not Before");

			break;
		case ValidityNotAfterField:
			title = tr("Not After");

			break;
		case SubjectField:
			title = tr("Subject");

			break;
		case PublicKeyField:
			title = tr("Subject Public Key");

			break;
		case PublicKeyAlgorithmField:
			title = tr("Algorithm");

			break;
		case PublicKeyValueField:
			title = tr("Public Key");

			break;
		case ExtensionsField:
			title = tr("Extensions");

			break;
		case ExtensionField:
			title = data.value(Qt::DisplayRole, tr("Unknown")).toString();

			break;
		case DigestField:
			title = tr("Digest");

			break;
		case DigestSha1Field:
			title = tr("SHA-1 Digest");

			break;
		case DigestSha256Field:
			title = tr("SHA-256 Digest");

			break;
		default:
			break;
	}

	QStandardItem *item = new QStandardItem(title);
	item->setData(field, Qt::UserRole);

	QMap<int, QVariant>::const_iterator iterator;

	for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator)
	{
		item->setData(iterator.value(), iterator.key());
	}

	if (parent)
	{
		parent->appendRow(item);
	}
	else
	{
		m_ui->detailsItemView->getModel()->appendRow(item);
	}

	return item;
}

}
