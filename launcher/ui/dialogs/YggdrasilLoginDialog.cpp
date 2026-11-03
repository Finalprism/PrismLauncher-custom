// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */

#include "YggdrasilLoginDialog.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

#include <Application.h>
#include <FileSystem.h>

#include <QDebug>

namespace {
constexpr const char* kDefaultServer = "https://littleskin.cn/api/yggdrasil";

/** Fire-and-forget: fetch authlib-injector.jar (from Maven Central) into the data root. */
void downloadAuthlibInjector()
{
    const QString jarPath = FS::PathCombine(APPLICATION->dataRoot(), QStringLiteral("authlib-injector.jar"));
    if (QFile::exists(jarPath))
        return;

    auto* nam = new QNetworkAccessManager(qApp);
    QNetworkRequest metaReq{ QUrl(QStringLiteral("https://repo.maven.apache.org/maven2/org/glavo/hmcl/authlib-injector/maven-metadata.xml")) };
    auto* metaReply = nam->get(metaReq);
    QObject::connect(metaReply, &QNetworkReply::finished, nam, [nam, metaReply, jarPath] {
        metaReply->deleteLater();
        if (metaReply->error() != QNetworkReply::NoError) {
            qWarning() << "authlib-injector metadata:" << metaReply->errorString();
            nam->deleteLater();
            return;
        }
        const QString xml = QString::fromUtf8(metaReply->readAll());
        QRegularExpressionMatch m = QRegularExpression(QStringLiteral("<latest>([^<]+)</latest>")).match(xml);
        if (!m.hasMatch())
            m = QRegularExpression(QStringLiteral("<release>([^<]+)</release>")).match(xml);
        if (!m.hasMatch()) {
            qWarning() << "authlib-injector: no version found in metadata";
            nam->deleteLater();
            return;
        }
        const QString ver = m.captured(1);
        QNetworkRequest jarReq{ QUrl(QStringLiteral("https://repo.maven.apache.org/maven2/org/glavo/hmcl/authlib-injector/%1/authlib-injector-%1.jar").arg(ver)) };
        auto* jarReply = nam->get(jarReq);
        QObject::connect(jarReply, &QNetworkReply::finished, nam, [nam, jarReply, jarPath] {
            jarReply->deleteLater();
            if (jarReply->error() != QNetworkReply::NoError) {
                qWarning() << "authlib-injector download:" << jarReply->errorString();
                nam->deleteLater();
                return;
            }
            QFile out(jarPath);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(jarReply->readAll());
                out.close();
                qInfo() << "authlib-injector saved to" << jarPath;
            } else {
                qWarning() << "authlib-injector: cannot write" << jarPath << out.errorString();
            }
            nam->deleteLater();
        });
    });
}
}  // namespace

YggdrasilLoginDialog::YggdrasilLoginDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("添加外置账户"));
    resize(460, 220);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    m_serverEdit = new QLineEdit(QString::fromLatin1(kDefaultServer), this);
    m_serverEdit->setPlaceholderText(tr("认证服务器地址"));
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText(tr("邮箱或用户名"));
    m_passEdit = new QLineEdit(this);
    m_passEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("服务器地址："), m_serverEdit);
    form->addRow(tr("用户名："), m_userEdit);
    form->addRow(tr("密码："), m_passEdit);
    layout->addLayout(form);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_loginButton = buttons->button(QDialogButtonBox::Ok);
    m_loginButton->setText(tr("登录"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &YggdrasilLoginDialog::tryLogin);
    connect(buttons, &QDialogButtonBox::rejected, this, &YggdrasilLoginDialog::reject);
}

MinecraftAccountPtr YggdrasilLoginDialog::newAccount(QWidget* parent)
{
    YggdrasilLoginDialog dialog(parent);
    if (dialog.exec() != QDialog::Accepted || !dialog.m_account)
        return nullptr;
    downloadAuthlibInjector();
    return dialog.m_account;
}

void YggdrasilLoginDialog::tryLogin()
{
    QString server = m_serverEdit->text().trimmed();
    while (server.endsWith(QLatin1Char('/')))
        server.chop(1);
    const QString username = m_userEdit->text().trimmed();
    const QString password = m_passEdit->text();

    if (server.isEmpty() || !server.startsWith(QStringLiteral("http"))) {
        m_statusLabel->setText(tr("请输入有效的服务器地址。"));
        m_statusLabel->setVisible(true);
        return;
    }
    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(tr("请输入用户名和密码。"));
        m_statusLabel->setVisible(true);
        return;
    }

    auto account = MinecraftAccount::createYggdrasil(server, username, password);
    m_task = account->login();
    connect(m_task.get(), &Task::succeeded, this, [this, account] {
        m_account = account;
        accept();
    });
    connect(m_task.get(), &Task::failed, this, &YggdrasilLoginDialog::onTaskFailed);
    connect(m_task.get(), &Task::aborted, this, [this] { onTaskFailed(tr("已取消")); });

    m_loginButton->setEnabled(false);
    m_statusLabel->setText(tr("正在登录…"));
    m_statusLabel->setVisible(true);
    m_task->start();
}

void YggdrasilLoginDialog::onTaskFailed(QString reason)
{
    m_task.reset();
    m_loginButton->setEnabled(true);
    m_statusLabel->setText(tr("登录失败：%1").arg(reason));
    m_statusLabel->setVisible(true);
}

void YggdrasilLoginDialog::onTaskStatus(QString status)
{
    m_statusLabel->setText(status);
}
