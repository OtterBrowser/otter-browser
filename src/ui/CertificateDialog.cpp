/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2017 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "CertificateDialog.h"
#include "../core/Utils.h"

#include "ui_CertificateDialog.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtNetwork/QSslCertificateExtension>
#include <QtNetwork/QSslKey>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

namespace Otter
{

CertificateDialog::CertificateDialog(QVector<QSslCertificate> certificates, QWidget *parent) : Dialog(parent),
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

	QStandardItemModel *chainModel(new QStandardItemModel(this));
	QStandardItem *certificateItem(nullptr);

	for (int i = (certificates.count() - 1); i >= 0; --i)
	{
		QStandardItem *parentCertificateItem(certificateItem);

		certificateItem = new QStandardItem(certificates.at(i).subjectInfo(QSslCertificate::Organization).value(0, tr("Unknown")));
		certificateItem->setData(i, CertificateIndexRole);

		if (parentCertificateItem)
		{
			parentCertificateItem->appendRow(certificateItem);
		}
		else
		{
			chainModel->appendRow(certificateItem);
		}
	}

	m_ui->chainItemView->setViewMode(ItemViewWidget::TreeView);
	m_ui->chainItemView->setModel(chainModel);
	m_ui->chainItemView->expandAll();
	m_ui->chainItemView->setCurrentIndex(chainModel->index(0, 0));
	m_ui->detailsItemView->setViewMode(ItemViewWidget::TreeView);
	m_ui->detailsItemView->setModel(new QStandardItemModel(this));

	updateCertificate();

	connect(m_ui->chainItemView, &ItemViewWidget::needsActionsUpdate, this, &CertificateDialog::updateCertificate);
	connect(m_ui->detailsItemView, &ItemViewWidget::needsActionsUpdate, this, &CertificateDialog::updateValue);
	connect(m_ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CertificateDialog::exportCertificate);
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

		updateCertificate();
	}
}

void CertificateDialog::exportCertificate()
{
	const QSslCertificate certificate(m_certificates.value(m_ui->chainItemView->currentIndex().data(CertificateIndexRole).toInt()));

	if (certificate.isNull())
	{
		return;
	}

	QString filter;
	const QString path(QFileDialog::getSaveFileName(this, tr("Select File"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0), Utils::formatFileTypes({tr("DER encoded X.509 certificates (*.der)"), tr("PEM encoded X.509 certificates (*.pem)"), tr("Text files (*.txt)")}), &filter));

	if (!path.isEmpty())
	{
		QFile file(path);

		if (!file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing."), QMessageBox::Close);

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

void CertificateDialog::updateCertificate()
{
	const QSslCertificate certificate(m_certificates.value(m_ui->chainItemView->currentIndex().data(CertificateIndexRole).toInt()));
	const QVariant field(m_ui->detailsItemView->currentIndex().data(CertificateFieldRole));
	const QVariant extension(m_ui->detailsItemView->currentIndex().data(ExtensionNameRole));

	m_ui->detailsItemView->getSourceModel()->clear();

	createField(VersionField);
	createField(SerialNumberField);
	createField(SignatureAlgorithmField);
	createField(IssuerField);

	QStandardItem *validityItem(createField(ValidityField));
	validityItem->setFlags(Qt::ItemIsEnabled);

	createField(ValidityNotBeforeField, validityItem);
	createField(ValidityNotAfterField, validityItem);
	createField(SubjectField);

	QStandardItem *publicKeyItem(createField(PublicKeyField));
	publicKeyItem->setFlags(Qt::ItemIsEnabled);

	createField(PublicKeyAlgorithmField, publicKeyItem);
	createField(PublicKeyValueField, publicKeyItem);

	QStandardItem *extensionsItem(createField(ExtensionsField));
	extensionsItem->setFlags(Qt::ItemIsEnabled);
	extensionsItem->setEnabled(certificate.extensions().count() > 0);

	for (int i = 0; i < certificate.extensions().count(); ++i)
	{
		QString title(certificate.extensions().at(i).name());

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
			title = tr("Inhibit Any Policy");
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

		createField(ExtensionField, extensionsItem, {{Qt::DisplayRole, title}, {ExtensionIndexRole, i}, {ExtensionNameRole, certificate.extensions().at(i).name()}});
	}

	QStandardItem *digestItem(createField(DigestField));

	createField(DigestSha256Field, digestItem);
	createField(DigestSha1Field, digestItem);

	m_ui->detailsItemView->expandAll();
	m_ui->valueTextEditWidget->clear();

	if (!field.isNull())
	{
		const bool isExtension(static_cast<CertificateField>(field.toInt()) == ExtensionField);
		const QModelIndexList indexes(m_ui->detailsItemView->getSourceModel()->match(m_ui->detailsItemView->getSourceModel()->index(0, 0), (isExtension ? ExtensionNameRole : CertificateFieldRole), (isExtension ? extension : field), 1, (Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap)));

		if (!indexes.isEmpty())
		{
			m_ui->detailsItemView->setCurrentIndex(indexes.first());
		}
	}
}

void CertificateDialog::updateValue()
{
	const QSslCertificate certificate(m_certificates.value(m_ui->chainItemView->currentIndex().data(CertificateIndexRole).toInt()));
	const CertificateField field(static_cast<CertificateField>(m_ui->detailsItemView->currentIndex().data(CertificateFieldRole).toInt()));

	m_ui->valueTextEditWidget->clear();

	switch (field)
	{
		case ValidityField:
		case PublicKeyField:
		case ExtensionsField:
		case DigestField:
			break;
		case VersionField:
			m_ui->valueTextEditWidget->setPlainText(QString(certificate.version()));

			break;
		case SerialNumberField:
			m_ui->valueTextEditWidget->setPlainText(formatHex(QString(certificate.serialNumber()), QLatin1Char(':')));

			break;
		case SignatureAlgorithmField:
			m_ui->valueTextEditWidget->setPlainText(QRegularExpression(QLatin1String("Signature Algorithm:(.+)")).match(certificate.toText()).captured(1).trimmed());

			break;
		case IssuerField:
			{
				const QList<QByteArray> attributes(certificate.issuerInfoAttributes());

				for (int i = 0; i < attributes.count(); ++i)
				{
					m_ui->valueTextEditWidget->appendPlainText(QStringLiteral("%1 = %2").arg(QString(attributes.at(i))).arg(certificate.issuerInfo(attributes.at(i)).join(QLatin1String(", "))));
				}
			}

			break;
		case ValidityNotBeforeField:
			m_ui->valueTextEditWidget->setPlainText(certificate.effectiveDate().toString(QLatin1String("yyyy-MM-dd hh:mm:ss t")));

			break;
		case ValidityNotAfterField:
			m_ui->valueTextEditWidget->setPlainText(certificate.expiryDate().toString(QLatin1String("yyyy-MM-dd hh:mm:ss t")));

			break;
		case SubjectField:
			{
				const QList<QByteArray> attributes(certificate.subjectInfoAttributes());

				for (int i = 0; i < attributes.count(); ++i)
				{
					m_ui->valueTextEditWidget->appendPlainText(QStringLiteral("%1 = %2").arg(QString(attributes.at(i))).arg(certificate.subjectInfo(attributes.at(i)).join(QLatin1String(", "))));
				}
			}

			break;
		case PublicKeyValueField:
			{
				const QRegularExpression expression(QLatin1String("Public-Key:[.\\s\\S]+Modulus:([.\\s\\S]+)Exponent:(.+)"), QRegularExpression::MultilineOption);
				const QRegularExpressionMatch match(expression.match(certificate.toText()));

				if (match.hasMatch())
				{
					m_ui->valueTextEditWidget->setPlainText(tr("Modulus:\n%1\n\nExponent: %2").arg(formatHex(match.captured(1).trimmed().mid(3))).arg(match.captured(2).trimmed()));
				}
			}

			break;
		case PublicKeyAlgorithmField:
			m_ui->valueTextEditWidget->setPlainText(QRegularExpression(QLatin1String("Public Key Algorithm:(.+)")).match(certificate.toText()).captured(1).trimmed());

			break;
		case ExtensionField:
			{
				const QSslCertificateExtension extension(certificate.extensions().value(m_ui->detailsItemView->currentIndex().data(ExtensionIndexRole).toInt()));

				m_ui->valueTextEditWidget->setPlainText(extension.isCritical() ? tr("Critical") : tr("Not Critical"));
				m_ui->valueTextEditWidget->appendPlainText(tr("OID: %1").arg(extension.oid()));

				if (!extension.value().isNull())
				{
					m_ui->valueTextEditWidget->appendPlainText(tr("Value:"));

					if (extension.value().type() == QVariant::List)
					{
						const QVariantList list(extension.value().toList());

						for (int i = 0; i < list.count(); ++i)
						{
							m_ui->valueTextEditWidget->appendPlainText(list.at(i).toString());
						}
					}
					else if (extension.value().type() == QVariant::Map)
					{
						const QVariantMap map(extension.value().toMap());
						QVariantMap::const_iterator iterator;

						for (iterator = map.constBegin(); iterator != map.constEnd(); ++iterator)
						{
							m_ui->valueTextEditWidget->appendPlainText(QStringLiteral("%1 = %2").arg(iterator.key()).arg(iterator.value().toString()));
						}
					}
					else
					{
						m_ui->valueTextEditWidget->appendPlainText(extension.value().toString());
					}
				}
			}

			break;
		case DigestSha1Field:
			m_ui->valueTextEditWidget->setPlainText(formatHex(QString(certificate.digest(QCryptographicHash::Sha1).toHex())));

			break;
		case DigestSha256Field:
			m_ui->valueTextEditWidget->setPlainText(formatHex(QString(certificate.digest(QCryptographicHash::Sha256).toHex())));

			break;
		default:
			break;
	}

	QTextCursor cursor(m_ui->valueTextEditWidget->textCursor());
	cursor.setPosition(0);

	m_ui->valueTextEditWidget->setTextCursor(cursor);
}

QStandardItem* CertificateDialog::createField(CertificateDialog::CertificateField field, QStandardItem *parent, const QMap<int, QVariant> &metaData)
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
			title = metaData.value(Qt::DisplayRole, tr("Unknown")).toString();

			break;
		case DigestField:
			title = tr("Fingerprint");

			break;
		case DigestSha1Field:
			title = tr("SHA-1 Fingerprint");

			break;
		case DigestSha256Field:
			title = tr("SHA-256 Fingerprint");

			break;
		default:
			break;
	}

	QStandardItem *item(new QStandardItem(title));
	item->setData(field, CertificateFieldRole);

	QMap<int, QVariant>::const_iterator iterator;

	for (iterator = metaData.constBegin(); iterator != metaData.constEnd(); ++iterator)
	{
		item->setData(iterator.value(), iterator.key());
	}

	if (parent)
	{
		parent->appendRow(item);
	}
	else
	{
		m_ui->detailsItemView->getSourceModel()->appendRow(item);
	}

	return item;
}

QString CertificateDialog::formatHex(const QString &source, const QChar &separator)
{
	QString result;
	int characterCount(0);
	int pairCount(0);

	for (int i = 0; i < source.length(); ++i)
	{
		if (source.at(i).isLetterOrNumber())
		{
			result.append(source.at(i));

			++characterCount;

			if (characterCount == 2)
			{
				++pairCount;

				if (pairCount == 16)
				{
					result.append(QLatin1Char('\n'));

					pairCount = 0;
				}
				else
				{
					result.append(separator);
				}

				characterCount = 0;
			}
		}
	}

	return result.trimmed().toUpper();
}

}
