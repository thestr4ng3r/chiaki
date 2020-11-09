// SPDX-License-Identifier: LicenseRef-GPL-3.0-or-later-OpenSSL

#ifndef CHIAKI_SETTINGSDIALOG_H
#define CHIAKI_SETTINGSDIALOG_H

#include <QDialog>

class Settings;
class QListWidget;
class QComboBox;
class QCheckBox;
class QLineEdit;

class SettingsDialog : public QDialog
{
	Q_OBJECT

	private:
		Settings *settings;

		QCheckBox *log_verbose_check_box;
		QComboBox *disconnect_action_combo_box;

		QComboBox *resolution_combo_box;
		QComboBox *fps_combo_box;
		QLineEdit *bitrate_edit;
		QLineEdit *audio_buffer_size_edit;
		QComboBox *hardware_decode_combo_box;

		QListWidget *registered_hosts_list_widget;
		QPushButton *delete_registered_host_button;

		void UpdateBitratePlaceholder();

	private slots:
		void LogVerboseChanged();
		void DisconnectActionSelected();

		void ResolutionSelected();
		void FPSSelected();
		void BitrateEdited();
		void AudioBufferSizeEdited();
		void HardwareDecodeEngineSelected();

		void UpdateRegisteredHosts();
		void UpdateRegisteredHostsButtons();
		void RegisterNewHost();
		void DeleteRegisteredHost();

	public:
		SettingsDialog(Settings *settings, QWidget *parent = nullptr);
};

#endif // CHIAKI_SETTINGSDIALOG_H
