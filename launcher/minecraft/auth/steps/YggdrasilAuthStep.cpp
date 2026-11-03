// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */

#include "YggdrasilAuthStep.h"

#include "minecraft/auth/Parsers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#include <QDebug>

YggdrasilAuthStep::YggdrasilAuthStep(AccountData* data, bool isRefresh) : AuthStep(data), m_isRefresh(isRefresh) {}

QString YggdrasilAuthStep::describe()
{
    return m_isRefresh ? tr("正在刷新外置登录会话…") : tr("正在登录外置认证服务器…");
}

void YggdrasilAuthStep::perform()
{
    const QString server = m_data->yggdrasilServer;
    if (server.isEmpty()) {
        failWith(AccountTaskState::STATE_FAILED_HARD, tr("外置认证服务器地址为空。"));
        return;
    }

    QJsonObject payload;
    if (m_isRefresh) {
        payload["accessToken"] = m_data->yggdrasilToken.token;
        payload["clientToken"] = m_data->yggdrasilToken.extra.value("clientToken").toString();
    } else {
        if (m_data->yggdrasilPassword.isEmpty()) {
            failWith(AccountTaskState::STATE_FAILED_HARD, tr("缺少密码。"));
            return;
        }
        QJsonObject agent;
        agent["name"] = "Minecraft";
        agent["version"] = 1;
        payload["agent"] = agent;
        payload["username"] = m_data->yggdrasilToken.extra.value("userName").toString();
        payload["password"] = m_data->yggdrasilPassword;
        payload["clientToken"] = m_data->yggdrasilToken.extra.value("clientToken").toString();
    }
    payload["requestUser"] = true;

    QUrl url(server + (m_isRefresh ? "/authserver/refresh" : "/authserver/authenticate"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &YggdrasilAuthStep::onRequestFinished);
}

void YggdrasilAuthStep::abort()
{
    if (m_reply) {
        m_reply->abort();
    }
}

void YggdrasilAuthStep::onRequestFinished()
{
    auto* reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto body = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        if (status == 403 || status == 401) {
            failWith(AccountTaskState::STATE_FAILED_HARD, tr("用户名或密码错误（HTTP %1）。").arg(status));
        } else if (status >= 500) {
            failWith(AccountTaskState::STATE_OFFLINE, tr("认证服务器错误（HTTP %1）。").arg(status));
        } else if (!body.isEmpty()) {
            failWith(AccountTaskState::STATE_FAILED_HARD, QString::fromUtf8(body.left(300)));
        } else {
            failWith(AccountTaskState::STATE_OFFLINE, reply->errorString());
        }
        return;
    }

    QJsonParseError parseError{};
    const auto doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        failWith(AccountTaskState::STATE_FAILED_SOFT, tr("认证服务器返回了无效响应。"));
        return;
    }
    succeedWith(doc.object());
}

void YggdrasilAuthStep::succeedWith(const QJsonObject& root)
{
    const QString accessToken = root.value("accessToken").toString();
    if (accessToken.isEmpty()) {
        failWith(AccountTaskState::STATE_FAILED_SOFT, tr("认证服务器未返回访问令牌。"));
        return;
    }

    QJsonObject profile = root.value("selectedProfile").toObject();
    if (profile.isEmpty()) {
        const auto available = root.value("availableProfiles").toArray();
        if (!available.isEmpty()) {
            profile = available.first().toObject();
        }
    }
    const QString profileId = profile.value("id").toString();
    const QString profileName = profile.value("name").toString();
    if (profileId.isEmpty() || profileName.isEmpty()) {
        failWith(AccountTaskState::STATE_FAILED_SOFT, tr("该账户在此服务器上没有 Minecraft 角色。"));
        return;
    }

    const QString respClient = root.value("clientToken").toString();
    if (!respClient.isEmpty()) {
        m_data->yggdrasilToken.extra["clientToken"] = respClient;
    }

    const auto now = QDateTime::currentDateTimeUtc();
    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.issueInstant = now;
    m_data->yggdrasilToken.notAfter = now.addSecs(24 * 3600);
    m_data->yggdrasilToken.validity = Validity::Certain;
    m_data->yggdrasilPassword.clear();

    m_data->minecraftProfile.id = profileId;
    m_data->minecraftProfile.name = profileName;
    m_data->minecraftProfile.validity = Validity::Certain;

    // external auth grants the same play entitlement as a purchased copy
    m_data->minecraftEntitlement.ownsMinecraft = true;
    m_data->minecraftEntitlement.canPlayMinecraft = true;
    m_data->minecraftEntitlement.validity = Validity::Certain;

    // Mojang's sessionserver knows nothing about external-auth UUIDs; pull
    // textures from the auth server's own sessionserver (best effort).
    QNetworkRequest profileReq{ QUrl(m_data->yggdrasilServer +
                                      QStringLiteral("/sessionserver/session/minecraft/profile/") + profileId) };
    m_reply = m_nam.get(profileReq);
    connect(m_reply, &QNetworkReply::finished, this, &YggdrasilAuthStep::onProfileFinished);
}

void YggdrasilAuthStep::failWith(AccountTaskState state, const QString& message)
{
    qWarning() << "YggdrasilAuthStep:" << message;
    emit finished(state, message);
}

void YggdrasilAuthStep::onProfileFinished()
{
    if (m_reply) {
        const auto err = m_reply->error();
        auto body = m_reply->readAll();
        const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_reply->deleteLater();
        m_reply = nullptr;
        if (err == QNetworkReply::OperationCanceledError) {
            failWith(AccountTaskState::STATE_OFFLINE, tr("已取消。"));
            return;
        }
        if (err == QNetworkReply::NoError && status == 200 &&
            Parsers::parseMinecraftProfileMojang(body, m_data->minecraftProfile)) {
            m_data->minecraftProfile.validity = Validity::Certain;
        } else {
            qWarning() << "YggdrasilAuthStep: sessionserver profile fetch failed (status" << status
                       << "), skin stays default";
        }
    }
    emit finished(AccountTaskState::STATE_SUCCEEDED, tr("已登录外置认证服务器。"));
}
