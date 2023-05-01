#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <cassert>

#include <QDebug>

#include "settingsmanager.hpp"

/**
 * MountSettings is the settings manager for this application.
 * It uses the generic SettingsManager class (for loading/saving JSON)
 * and adds application-specific methods.
 *
 * Like SettingsManager, it can either be used locally (on the stack)
 * or globally, in case the config file should only be loaded once,
 * providing access to the loaded configuration,
 * which would then be saved on demand, when the user clicks on "save"
 * or in some close event.
 */
class Settings : public SettingsManager
{

signals:

public:

/*
    static Settings*
    globalInstance();

    MountSettings();

    MountSettings(const MountSettings &other);

    QList<QVariantMap>
    mountConfigList() const;

    QVariantMap
    mountConfig(const QString &mountpoint) const;

    void
    setMountConfig(const QString &mountpoint, const QVariantMap &config);

    void
    setMountConfig(const QVariantMap &config);

    void
    removeMountConfig(const QString &mountpoint);

    void
    setMountConfigList(const QList<QVariantMap> &config);
*/

private:

};

#endif
