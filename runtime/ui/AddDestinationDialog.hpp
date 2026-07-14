#pragma once

#include "../engine/config/OutputProfile.hpp"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>
#include <string>

namespace mskit::ui {

class AddDestinationDialog : public QDialog {
    Q_OBJECT

private:
    // Core Layout Input Blocks
    QLineEdit* id_input = nullptr;
    QComboBox* protocol_combo = nullptr;
    QLineEdit* url_input = nullptr;
    QLineEdit* key_input = nullptr;
    QLabel* key_label = nullptr;

    // Hardware/Encoding Constraints Blocks
    QSpinBox* bitrate_spin = nullptr;
    QSpinBox* width_spin = nullptr;
    QSpinBox* height_spin = nullptr;
    QComboBox* preset_combo = nullptr;

    QDialogButtonBox* button_box = nullptr;

    void SetupLayouts();
    void ConnectSignals();

private slots:
    // Responds dynamically to layout parameter modifications
    void HandleProtocolChanged(int index);
    void ValidateAndAccept();

public:
    explicit AddDestinationDialog(QWidget* parent = nullptr);
    virtual ~AddDestinationDialog() override = default;

    // Type-safe string conversions for clean protocol binding loops
    std::string GetNodeId() const { return id_input->text().trimmed().toStdString(); }
    std::string GetProtocol() const { return protocol_combo->currentText().toStdString(); }
    std::string GetUrl() const { return url_input->text().trimmed().toStdString(); }
    std::string GetStreamKey() const { return key_input->text().toStdString(); }

    // Assembles the localized control snapshot parameters into an immutable core profile
    mskit::OutputProfile GetProfile() const;
};

} // namespace mskit::ui
