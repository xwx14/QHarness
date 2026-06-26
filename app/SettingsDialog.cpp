#include "SettingsDialog.h"
#include <QTabWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>

namespace qh {
namespace app {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("设置");
    resize(620, 460);

    _tabs = new QTabWidget(this);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->addWidget(_tabs);

    // ===== Tab 1: LLM 配置 =====
    auto* llm = new QWidget;
    auto* llmH = new QHBoxLayout(llm);
    _profileList = new QListWidget;
    auto* right = new QWidget;
    auto* form = new QFormLayout(right);
    _nameEdit       = new QLineEdit;
    _typeCombo      = new QComboBox;
    _typeCombo->addItem("OpenAI", (int)schema::ProviderType::OpenAI);
    _typeCombo->addItem("Claude", (int)schema::ProviderType::Claude);
    _baseUrlEdit    = new QLineEdit;
    _apiKeyEdit     = new QLineEdit;
    _apiKeyEdit->setEchoMode(QLineEdit::Password);
    _modelEdit      = new QLineEdit;
    _tempEdit       = new QLineEdit;   // 空 = 不下发
    _maxTokensEdit  = new QLineEdit;
    form->addRow("名称", _nameEdit);
    form->addRow("类型", _typeCombo);
    form->addRow("Base URL", _baseUrlEdit);
    form->addRow("API Key", _apiKeyEdit);
    form->addRow("Model", _modelEdit);
    form->addRow("Temperature", _tempEdit);
    form->addRow("Max Tokens", _maxTokensEdit);
    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton("新增");
    auto* delBtn = new QPushButton("删除");
    auto* setActiveBtn = new QPushButton("设为当前");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(delBtn);
    btnRow->addStretch();
    btnRow->addWidget(setActiveBtn);
    auto* rightV = new QVBoxLayout;
    rightV->addLayout(form);
    rightV->addLayout(btnRow);
    rightV->addStretch();
    right->setLayout(rightV);
    llmH->addWidget(_profileList);
    llmH->addWidget(right, 1);
    _tabs->addTab(llm, "LLM 配置");

    connect(addBtn, &QPushButton::clicked, this, &SettingsDialog::onAddProfile);
    connect(delBtn, &QPushButton::clicked, this, &SettingsDialog::onDelProfile);
    connect(setActiveBtn, &QPushButton::clicked, this, &SettingsDialog::onSetActive);
    connect(_profileList, &QListWidget::currentRowChanged, this,
            [this](int){ onProfileSelected(); });
    // 表单任意改动 → 回写当前 profile
    for (auto* le : {_nameEdit, _baseUrlEdit, _apiKeyEdit, _modelEdit, _tempEdit, _maxTokensEdit}) {
        connect(le, &QLineEdit::textChanged, this, &SettingsDialog::onEditChanged);
    }
    connect(_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onEditChanged(); });

    // ===== Tab 2: 引擎 =====
    auto* eng = new QWidget;
    auto* engV = new QVBoxLayout(eng);
    _thinkingCheck = new QCheckBox("开启慢思考（两阶段引擎：剥夺工具的纯文本规划 Phase1）");
    engV->addWidget(_thinkingCheck);
    engV->addStretch();
    _tabs->addTab(eng, "引擎");

    // ===== Tab 3: 工具 =====
    auto* tools = new QWidget;
    auto* toolsV = new QVBoxLayout(tools);
    _toolsList = new QListWidget;
    _toolsList->addItem("bash");   // 本阶段唯一可用工具
    QListWidgetItem* it = _toolsList->item(0);
    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
    it->setCheckState(Qt::Unchecked);
    toolsV->addWidget(new QLabel("勾选要加载的工具："));
    toolsV->addWidget(_toolsList);
    _tabs->addTab(tools, "工具");

    // ===== Tab 4: 工作目录 =====
    auto* wd = new QWidget;
    auto* wdH = new QHBoxLayout(wd);
    _workDirEdit = new QLineEdit;
    auto* browse = new QPushButton("浏览…");
    wdH->addWidget(_workDirEdit);
    wdH->addWidget(browse);
    _tabs->addTab(wd, "工作目录");
    connect(browse, &QPushButton::clicked, this, &SettingsDialog::onPickWorkDir);

    // ===== 底部确定/取消 =====
    auto* bottom = new QHBoxLayout;
    bottom->addStretch();
    auto* okBtn = new QPushButton("确定");
    auto* cancelBtn = new QPushButton("取消");
    bottom->addWidget(okBtn);
    bottom->addWidget(cancelBtn);
    root->addLayout(bottom);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::setSettings(const schema::Settings& s) {
    _settings = s;
    _loading = true;
    _thinkingCheck->setChecked(s._enableThinking);
    _workDirEdit->setText(QString::fromStdString(s._workDir));
    // 工具勾选
    QListWidgetItem* bashItem = _toolsList->item(0);
    bool bashOn = std::find(s._enabledTools.begin(), s._enabledTools.end(), "bash")
                  != s._enabledTools.end();
    bashItem->setCheckState(bashOn ? Qt::Checked : Qt::Unchecked);
    rebuildProfileList(s._profiles.empty() ? -1 : 0);
    _loading = false;
}

schema::Settings SettingsDialog::getSettings() const {
    schema::Settings s = _settings;
    s._enableThinking = _thinkingCheck->isChecked();
    s._workDir = _workDirEdit->text().trimmed().toStdString();
    // 工具
    s._enabledTools.clear();
    QListWidgetItem* bashItem = _toolsList->item(0);
    if (bashItem && bashItem->checkState() == Qt::Checked) {
        s._enabledTools.push_back("bash");
    }
    return s;
}

void SettingsDialog::rebuildProfileList(int selectRow) {
    _profileList->blockSignals(true);
    _profileList->clear();
    for (const auto& p : _settings._profiles) {
        QString label = QString::fromStdString(p._name);
        if (p._name == _settings._activeProfileName) label += "  ✓当前";
        _profileList->addItem(label);
    }
    _profileList->blockSignals(false);
    if (selectRow >= 0 && selectRow < _profileList->count()) {
        _profileList->setCurrentRow(selectRow);
        loadProfileToForm(selectRow);
    } else {
        loadProfileToForm(-1);
    }
}

void SettingsDialog::loadProfileToForm(int idx) {
    _loading = true;
    if (idx < 0 || idx >= (int)_settings._profiles.size()) {
        _nameEdit->clear(); _baseUrlEdit->clear(); _apiKeyEdit->clear();
        _modelEdit->clear(); _tempEdit->clear(); _maxTokensEdit->clear();
        _typeCombo->setCurrentIndex(0);
        _loading = false;
        return;
    }
    const auto& p = _settings._profiles[idx];
    _nameEdit->setText(QString::fromStdString(p._name));
    _typeCombo->setCurrentIndex(p._providerType == schema::ProviderType::Claude ? 1 : 0);
    _baseUrlEdit->setText(QString::fromStdString(p._baseUrl));
    _apiKeyEdit->setText(QString::fromStdString(p._apiKey));
    _modelEdit->setText(QString::fromStdString(p._model));
    _tempEdit->setText(p._temperature ? QString::number(*p._temperature) : "");
    _maxTokensEdit->setText(p._maxTokens ? QString::number(*p._maxTokens) : "");
    _loading = false;
}

int SettingsDialog::currentRow() const {
    return _profileList->currentRow();
}

void SettingsDialog::onProfileSelected() {
    if (_loading) return;
    loadProfileToForm(currentRow());
}

void SettingsDialog::onEditChanged() {
    if (_loading) return;
    int idx = currentRow();
    if (idx < 0 || idx >= (int)_settings._profiles.size()) return;
    auto& p = _settings._profiles[idx];
    QString oldName = QString::fromStdString(p._name);
    p._name      = _nameEdit->text().trimmed().toStdString();
    p._providerType = static_cast<schema::ProviderType>(_typeCombo->currentData().toInt());
    p._baseUrl   = _baseUrlEdit->text().toStdString();
    p._apiKey    = _apiKeyEdit->text().toStdString();
    p._model     = _modelEdit->text().toStdString();
    bool tok = false;
    double t = _tempEdit->text().trimmed().toDouble(&tok);
    p._temperature = tok ? std::optional<double>(t) : std::nullopt;
    bool mok = false;
    int m = _maxTokensEdit->text().trimmed().toInt(&mok);
    p._maxTokens = mok ? std::optional<int>(m) : std::nullopt;
    // 名称变化 → 刷新列表显示
    if (oldName != QString::fromStdString(p._name)) {
        rebuildProfileList(idx);
    }
}

void SettingsDialog::onAddProfile() {
    schema::LlmProfile p;
    p._name = "新配置 " + std::to_string(_settings._profiles.size() + 1);
    p._providerType = schema::ProviderType::OpenAI;
    _settings._profiles.push_back(p);
    rebuildProfileList((int)_settings._profiles.size() - 1);
}

void SettingsDialog::onDelProfile() {
    int idx = currentRow();
    if (idx < 0 || idx >= (int)_settings._profiles.size()) return;
    std::string removed = _settings._profiles[idx]._name;
    _settings._profiles.erase(_settings._profiles.begin() + idx);
    if (_settings._activeProfileName == removed) {
        _settings._activeProfileName.clear();   // 删除激活项 → 取消激活
    }
    rebuildProfileList(_settings._profiles.empty() ? -1
                       : std::min(idx, (int)_settings._profiles.size() - 1));
}

void SettingsDialog::onSetActive() {
    int idx = currentRow();
    if (idx < 0 || idx >= (int)_settings._profiles.size()) return;
    _settings._activeProfileName = _settings._profiles[idx]._name;
    rebuildProfileList(idx);
}

void SettingsDialog::onPickWorkDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择工作目录",
                 _workDirEdit->text().isEmpty() ? QString() : _workDirEdit->text());
    if (!dir.isEmpty()) _workDirEdit->setText(dir);
}

} // namespace app
} // namespace qh
