#include "AddDestinationDialog.hpp"
#include <QVBoxLayout>
#include <QMessageBox>

namespace mskit::ui {

AddDestinationDialog::AddDestinationDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Configure New Stream Destination"));
    setMinimumWidth(420);
    setSizeGripEnabled(false);

    SetupLayouts();
    ConnectSignals();
}

void AddDestinationDialog::SetupLayouts() {
    auto* main_layout = new QVBoxLayout(this);
    auto* form_layout = new QFormLayout();
    form_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    id_input = new QLineEdit(this);
    id_input->setPlaceholderText(tr("e.g., twitch_backup"));

    protocol_combo = new QComboBox(this);
    protocol_combo->addItems({"RTMP", "SRT", "WHIP"});

    url_input = new QLineEdit(this);
    url_input->setPlaceholderText(tr("rtmp://live.twitch.tv/app"));

    key_input = new QLineEdit(this);
    key_input->setEchoMode(QLineEdit::Password);
    key_label = new QLabel(tr("Stream Key:"), this);

    // Audio/Video Processing Budget Inputs (ADR-007 constraints)
    bitrate_spin = new QSpinBox(this);
    bitrate_spin->setRange(1000, 25000);
    bitrate_spin->setValue(4500);
    bitrate_spin->setSuffix(" kbps");

    width_spin = new QSpinBox(this);
    width_spin->setRange(640, 7680);
    width_spin->setValue(1920);

    height_spin = new QSpinBox(this);
    height_spin->setRange(360, 4320);
    height_spin->setValue(1080);

    preset_combo = new QComboBox(this);
    preset_combo->addItems({"NVENC", "QSV", "AMF", "x264"});

    // Pack rows into form matrix panels
    form_layout->addRow(tr("Destination Token ID:"), id_input);
    form_layout->addRow(tr("Transport Protocol:"), protocol_combo);
    form_layout->addRow(tr("Target Ingest URL:"), url_input);
    form_layout->addRow(key_label, key_input);

    // UI Visual separator division boundary rule
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    form_layout->addRow(separator);

    form_layout->addRow(tr("Video Stream Bitrate:"), bitrate_spin);
    form_layout->addRow(tr("Output Frame Width:"), width_spin);
    form_layout->addRow(tr("Output Frame Height:"), height_spin);
    form_layout->addRow(tr("Hardware Profile Preset:"), preset_combo);

    button_box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);

    main_layout->addLayout(form_layout);
    main_layout->addWidget(button_box);
}

void AddDestinationDialog::ConnectSignals() {
    connect(protocol_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AddDestinationDialog::HandleProtocolChanged);
    connect(button_box, &QDialogButtonBox::accepted, this, &AddDestinationDialog::ValidateAndAccept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AddDestinationDialog::HandleProtocolChanged(int index) {
    QString protocol = protocol_combo->itemText(index);
    if (protocol == "SRT") {
        key_label->setText(tr("AES Passphrase (Optional):"));
        url_input->setPlaceholderText(tr("srt://ingest-server.net:9000"));
    } else if (protocol == "WHIP") {
        key_label->setText(tr("HTTP Bearer Token (Optional):"));
        url_input->setPlaceholderText(tr("https://whip.live-ingest.com/v1/endpoint"));
    } else { // RTMP
        key_label->setText(tr("Stream Key:"));
        url_input->setPlaceholderText(tr("rtmp://live.twitch.tv/app"));
    }
}

void AddDestinationDialog::ValidateAndAccept() {
    if (GetNodeId().empty()) {
        QMessageBox::warning(this, tr("Configuration Error"), tr("The unique Destination ID parameter field is required."));
        return;
    }
    if (GetUrl().empty()) {
        QMessageBox::warning(this, tr("Configuration Error"), tr("The target ingest URL destination field is required."));
        return;
    }
    accept(); // Closes structural loop returning code QDialog::Accepted
}

mskit::OutputProfile AddDestinationDialog::GetProfile() const {
    mskit::OutputProfile profile;
    profile.video_bitrate = static_cast<uint32_t>(bitrate_spin->value());
    profile.target_width = static_cast<uint32_t>(width_spin->value());
    profile.target_height = static_cast<uint32_t>(height_spin->value());
    profile.encoder_preset = preset_combo->currentText().toStdString();
    return profile;
}

} // namespace mskit::ui
