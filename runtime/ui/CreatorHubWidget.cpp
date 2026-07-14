#include "CreatorHubWidget.hpp"
#include "AddDestinationDialog.hpp"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <obs-module.h>

namespace mskit::ui {

CreatorHubWidget::CreatorHubWidget(std::shared_ptr<mskit::engine::OutputController> controller, QWidget* parent)
    : QWidget(parent), control_plane(controller) {

    SetupLayouts();
    ConfigureTableProperties();

    // Setup a 1000ms UI polling cycle to fulfill low-priority tracking specifications
    update_timer = new QTimer(this);
    connect(update_timer, &QTimer::timeout, this, &CreatorHubWidget::PollTelemetryUpdate);
    update_timer->start(1000);

    blog(LOG_INFO, "[MSK-UI] Creator Hub core dashboard layout initialized successfully.");
}

CreatorHubWidget::~CreatorHubWidget() {
    update_timer->stop();
}

void CreatorHubWidget::SetupLayouts() {
    main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->setSpacing(6);

    status_table = new QTableWidget(this);
    add_destination_btn = new QPushButton(tr("Add New Destination Target"), this);

    main_layout->addWidget(status_table);
    main_layout->addWidget(add_destination_btn);

    connect(add_destination_btn, &QPushButton::clicked, this, &CreatorHubWidget::HandleAddDestinationClick);
}

void CreatorHubWidget::ConfigureTableProperties() {
    status_table->setColumnCount(5);
    QStringList headers = { tr("Destination ID"), tr("Status"), tr("Bitrate"), tr("Framerate"), tr("RTT / Delay") };
    status_table->setHorizontalHeaderLabels(headers);

    status_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    status_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    status_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void CreatorHubWidget::PollTelemetryUpdate() {
    if (!control_plane) return;

    // Snapshot current engine active nodes via the reader-writer shared mutex lock bounds
    std::vector<std::shared_ptr<mskit::IOutputSession>> active_sessions = control_plane->GetAllActiveSessions();

    status_table->setRowCount(static_cast<int>(active_sessions.size()));

    int current_row = 0;
    for (const auto& session : active_sessions) {
        // Query streaming operational metrics from runtime tracking caches
        uint32_t bitrate_kbps = 0;
        uint32_t dropped_frames = 0;
        double live_fps = 0.0;
        session->GetLiveTelemetry(bitrate_kbps, live_fps, dropped_frames);

        int64_t rtt_ms = 0;
        double congestion_factor = 0.0;
        session->GetNetworkTelemetry(rtt_ms, congestion_factor);

        // Map textual state identifiers
        std::string status_str = "Inactive";
        if (session->IsActive()) {
            status_str = "Streaming";
            if (congestion_factor > 0.5) status_str = "Congested";
        }

        // Render cell data metrics frames smoothly to the UI elements grid
        status_table->setItem(current_row, 0, new QTableWidgetItem(QString::fromStdString(session->GetSessionId())));
        status_table->setItem(current_row, 1, new QTableWidgetItem(QString::fromStdString(status_str)));
        status_table->setItem(current_row, 2, new QTableWidgetItem(QString("%1 kbps").arg(bitrate_kbps)));
        status_table->setItem(current_row, 3, new QTableWidgetItem(QString("%1 FPS").arg(live_fps, 0, 'f', 1)));
        status_table->setItem(current_row, 4, new QTableWidgetItem(QString("%1 ms").arg(rtt_ms)));

        current_row++;
    }
}

void CreatorHubWidget::HandleAddDestinationClick() {
    AddDestinationDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        std::string node_id = dialog.GetNodeId();
        std::string protocol = dialog.GetProtocol();
        std::string url = dialog.GetUrl();
        std::string key = dialog.GetStreamKey();
        mskit::OutputProfile profile = dialog.GetProfile();

        if (control_plane) {
            // Actively spawn and register the session
            auto session = control_plane->CreateSession(node_id, protocol, url, key, profile);

            if (session) {
                blog(LOG_INFO, "[MSK-UI] Spawning target output session on the fly: %s", node_id.c_str());

                // Immediately kick off connection and encoding routines
                session->Open();
            } else {
                QMessageBox::critical(this, tr("Creation Failed"),
                    tr("Could not register the destination. Verify that the ID is unique."));
            }
        }
    }
}

} // namespace mskit::ui
