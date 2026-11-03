// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "minecraft/auth/AuthStep.h"

/**
 * Authentication step for Yggdrasil-compatible external auth servers
 * (authlib-injector, e.g. LittleSkin). Handles both login (authenticate)
 * and token refresh against {server}/authserver/*.
 */
class YggdrasilAuthStep : public AuthStep {
    Q_OBJECT

   public:
    explicit YggdrasilAuthStep(AccountData* data, bool isRefresh);

    QString describe() override;

   public slots:
    void perform() override;
    void abort() override;

   private slots:
    void onRequestFinished();

   private:
    void succeedWith(const QJsonObject& root);
    void failWith(AccountTaskState state, const QString& message);

    bool m_isRefresh;
    QNetworkAccessManager m_nam;
    QNetworkReply* m_reply = nullptr;
};
