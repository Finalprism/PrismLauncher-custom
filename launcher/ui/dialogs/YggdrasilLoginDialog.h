// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */

#pragma once

#include <QtWidgets/QDialog>

#include "minecraft/auth/AuthFlow.h"
#include "minecraft/auth/MinecraftAccount.h"

class QLabel;
class QLineEdit;

/**
 * Dialog for adding a Yggdrasil external-auth account
 * (server URL + username + password, e.g. LittleSkin).
 */
class YggdrasilLoginDialog : public QDialog {
    Q_OBJECT

   public:
    static MinecraftAccountPtr newAccount(QWidget* parent);

   private:
    explicit YggdrasilLoginDialog(QWidget* parent = nullptr);

   protected slots:
    void tryLogin();
    void onTaskFailed(QString reason);
    void onTaskStatus(QString status);

   private:
    MinecraftAccountPtr m_account;
    shared_qobject_ptr<AuthFlow> m_task;
    QLineEdit* m_serverEdit;
    QLineEdit* m_userEdit;
    QLineEdit* m_passEdit;
    QLabel* m_statusLabel;
    class QPushButton* m_loginButton;
};
