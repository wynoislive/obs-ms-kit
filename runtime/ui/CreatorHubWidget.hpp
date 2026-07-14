#pragma once

#include "../engine/output/OutputController.hpp"
#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QPushButton>
#include <memory>

namespace mskit::ui {

class CreatorHubWidget : public QWidget {
    Q_OBJECT

private:
    std::shared_ptr<mskit::engine::OutputController> control_plane;

    // Qt Graphical Components
    QVBoxLayout* main_layout = nullptr;
    QTableWidget* status_table = nullptr;
    QPushButton* add_destination_btn = nullptr;

    QTimer* update_timer = nullptr;

    void SetupLayouts();
    void ConfigureTableProperties();

private slots:
    // Core telemetry loop triggered asynchronously on the UI thread execution loop
    void PollTelemetryUpdate();
    void HandleAddDestinationClick();

public:
    explicit CreatorHubWidget(std::shared_ptr<mskit::engine::OutputController> controller, QWidget* parent = nullptr);
    virtual ~CreatorHubWidget() override;
};

} // namespace mskit::ui
