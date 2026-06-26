#include "SettingsDialog.h"
#include <QTabWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <algorithm>

namespace qh {
namespace app {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("设置");
    resize(720, 520);

    _tabs = new QTabWidget(this);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->addWidget(_tabs);

    // ===== Tab 1: LLM 配置 =====
    auto* llm = new QWidget;
    auto* llmH = new QHBoxLayout(llm);

    // 左：供应商列表
    auto* left = new QWidget;
    auto* leftV = new QVBoxLayout(left);
    leftV->addWidget(new QLabel("供应商："));
    _providerList = new QListWidget;
    leftV->addWidget(_providerList);
    auto* addProvBtn = new QPushButton("新增供应商");
    auto* delProvBtn = new QPushButton("删除供应商");
    auto* provBtnRow = new QHBoxLayout;
    provBtnRow->addWidget(addProvBtn);
    provBtnRow->addWidget(delProvBtn);
    leftV->addLayout(provBtnRow);
    leftV->addWidget(new QLabel("(单击列表项即设为当前供应商)"));
    llmH->addWidget(left);

    // 右：供应商设置 + 模型表格
    auto* right = new QWidget;
    auto* rightV = new QVBoxLayout(right);
    auto* form = new QFormLayout;
    _providerNameEdit = new QLineEdit;
    _typeCombo = new QComboBox;
    _typeCombo->addItem("OpenAI", (int)schema::ProviderType::OpenAI);
    _typeCombo->addItem("Claude", (int)schema::ProviderType::Claude);
    _baseUrlEdit = new QLineEdit;
    _apiKeyEdit  = new QLineEdit;
    _apiKeyEdit->setEchoMode(QLineEdit::Password);
    form->addRow("名称", _providerNameEdit);
    form->addRow("类型", _typeCombo);
    form->addRow("Base URL", _baseUrlEdit);
    form->addRow("API Key", _apiKeyEdit);
    rightV->addLayout(form);

    rightV->addWidget(new QLabel("模型（该供应商下）："));
    _modelTable = new QTableWidget;
    _modelTable->setColumnCount(4);
    _modelTable->setHorizontalHeaderLabels(QStringList() << "模型名" << "temperature" << "maxTokens" << "当前");
    _modelTable->horizontalHeader()->setStretchLastSection(true);
    _modelRadioGroup = new QButtonGroup(this);
    _modelRadioGroup->setExclusive(true);
    rightV->addWidget(_modelTable);
    auto* modelBtnRow = new QHBoxLayout;
    auto* addModelBtn = new QPushButton("添加模型");
    auto* delModelBtn = new QPushButton("删除模型");
    modelBtnRow->addWidget(addModelBtn);
    modelBtnRow->addWidget(delModelBtn);
    modelBtnRow->addStretch();
    rightV->addLayout(modelBtnRow);
    rightV->addStretch();
    llmH->addWidget(right, 1);
    _tabs->addTab(llm, "LLM 配置");

    connect(addProvBtn, &QPushButton::clicked, this, &SettingsDialog::onAddProvider);
    connect(delProvBtn, &QPushButton::clicked, this, &SettingsDialog::onDelProvider);
    connect(_providerList, &QListWidget::currentRowChanged, this,
            [this](int){ onProviderSelected(); });
    for (auto* le : {_providerNameEdit, _baseUrlEdit, _apiKeyEdit}) {
        connect(le, &QLineEdit::textChanged, this, &SettingsDialog::onProviderFieldChanged);
    }
    connect(_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onProviderFieldChanged(); });
    connect(addModelBtn, &QPushButton::clicked, this, &SettingsDialog::onAddModel);
    connect(delModelBtn, &QPushButton::clicked, this, &SettingsDialog::onDelModel);
    connect(_modelTable, &QTableWidget::cellChanged, this, &SettingsDialog::onModelCellChanged);

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
    _toolsList->addItem("bash");
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
    QListWidgetItem* bashItem = _toolsList->item(0);
    bool bashOn = std::find(s._enabledTools.begin(), s._enabledTools.end(), "bash")
                  != s._enabledTools.end();
    bashItem->setCheckState(bashOn ? Qt::Checked : Qt::Unchecked);
    // 选中激活供应商（单击语义：激活=当前选中）
    int row = -1;
    for (int i = 0; i < (int)_settings._providers.size(); ++i) {
        if (_settings._providers[i]._name == _settings._activeProviderName) { row = i; break; }
    }
    if (row < 0 && !_settings._providers.empty()) row = 0;
    rebuildProviderList(row);
    _loading = false;
}

schema::Settings SettingsDialog::getSettings() const {
    schema::Settings s = _settings;
    s._enableThinking = _thinkingCheck->isChecked();
    s._workDir = _workDirEdit->text().trimmed().toStdString();
    s._enabledTools.clear();
    QListWidgetItem* bashItem = _toolsList->item(0);
    if (bashItem && bashItem->checkState() == Qt::Checked) {
        s._enabledTools.push_back("bash");
    }
    return s;
}

int SettingsDialog::currentProviderRow() const { return _providerList->currentRow(); }

schema::LlmProvider* SettingsDialog::currentProvider() {
    int idx = _currentProviderIdx;
    if (idx < 0 || idx >= (int)_settings._providers.size()) return nullptr;
    return &_settings._providers[idx];
}

bool SettingsDialog::isProviderNameDuplicate(const std::string& name, int exceptIdx) const {
    for (int i = 0; i < (int)_settings._providers.size(); ++i) {
        if (i == exceptIdx) continue;
        if (_settings._providers[i]._name == name) return true;
    }
    return false;
}

bool SettingsDialog::isModelNameDuplicate(const std::string& name, int exceptIdx) const {
    auto* p = const_cast<SettingsDialog*>(this)->currentProvider();
    if (!p) return false;
    for (int i = 0; i < (int)p->_models.size(); ++i) {
        if (i == exceptIdx) continue;
        if (p->_models[i]._name == name) return true;
    }
    return false;
}

void SettingsDialog::rebuildProviderList(int selectRow) {
    _providerList->blockSignals(true);
    _providerList->clear();
    for (const auto& p : _settings._providers) {
        QString label = QString::fromStdString(p._name);
        if (p._name == _settings._activeProviderName) label += "  ✓当前";
        _providerList->addItem(label);
    }
    _providerList->blockSignals(false);
    if (selectRow >= 0 && selectRow < _providerList->count()) {
        _providerList->setCurrentRow(selectRow);   // 触发 onProviderSelected
    } else {
        _currentProviderIdx = -1;
        loadProviderToForm(-1);
    }
}

void SettingsDialog::loadProviderToForm(int idx) {
    _loading = true;
    _currentProviderIdx = idx;
    if (idx < 0 || idx >= (int)_settings._providers.size()) {
        _providerNameEdit->clear(); _baseUrlEdit->clear(); _apiKeyEdit->clear();
        _typeCombo->setCurrentIndex(0);
        rebuildModelTable();
        _loading = false;
        return;
    }
    const auto& p = _settings._providers[idx];
    _providerNameEdit->setText(QString::fromStdString(p._name));
    _typeCombo->setCurrentIndex(p._providerType == schema::ProviderType::Claude ? 1 : 0);
    _baseUrlEdit->setText(QString::fromStdString(p._baseUrl));
    _apiKeyEdit->setText(QString::fromStdString(p._apiKey));
    rebuildModelTable();
    _loading = false;
}

void SettingsDialog::clearModelTableSignals(bool block) {
    _modelTable->blockSignals(block);
}

void SettingsDialog::rebuildModelTable() {
    clearModelTableSignals(true);
    // 清旧 radio
    for (auto* b : _modelRadioGroup->buttons()) {
        _modelRadioGroup->removeButton(b);
        delete b;
    }
    auto* p = currentProvider();
    int rows = p ? (int)p->_models.size() : 0;
    _modelTable->setRowCount(rows);
    for (int r = 0; r < rows; ++r) {
        const auto& m = p->_models[r];
        auto* nameItem = new QTableWidgetItem(QString::fromStdString(m._name));
        auto* tempItem = new QTableWidgetItem(m._temperature ? QString::number(*m._temperature) : "");
        auto* maxItem  = new QTableWidgetItem(m._maxTokens ? QString::number(*m._maxTokens) : "");
        _modelTable->setItem(r, 0, nameItem);
        _modelTable->setItem(r, 1, tempItem);
        _modelTable->setItem(r, 2, maxItem);
        auto* radio = new QRadioButton;
        radio->setAutoExclusive(false);   // 由 QButtonGroup 管
        _modelTable->setCellWidget(r, 3, radio);
        _modelRadioGroup->addButton(radio, r);
        // 当前模型 radio 勾选
        if (p->_name == _settings._activeProviderName && m._name == _settings._activeModelName) {
            radio->setChecked(true);
        }
        connect(radio, &QRadioButton::toggled, this, [this, r](bool on){ onModelCurrentToggled(r, on); });
    }
    clearModelTableSignals(false);
}

void SettingsDialog::onProviderSelected() {
    if (_loading) return;
    int row = currentProviderRow();
    if (row < 0 || row >= (int)_settings._providers.size()) return;
    // 单击即设为当前供应商
    _settings._activeProviderName = _settings._providers[row]._name;
    loadProviderToForm(row);
    // 列表 ✓ 标记刷新
    _loading = true;
    _providerList->blockSignals(true);
    _providerList->clear();
    for (const auto& p : _settings._providers) {
        QString label = QString::fromStdString(p._name);
        if (p._name == _settings._activeProviderName) label += "  ✓当前";
        _providerList->addItem(label);
    }
    _providerList->blockSignals(false);
    _providerList->setCurrentRow(row);
    _loading = false;
    // 切供应商后当前模型可能不在新供应商下 → 清空
    if (!schema::findModel(_settings._providers[row], _settings._activeModelName)) {
        _settings._activeModelName.clear();
        clearModelTableSignals(true);
        for (auto* b : _modelRadioGroup->buttons()) b->setChecked(false);
        clearModelTableSignals(false);
    }
}

void SettingsDialog::onProviderFieldChanged() {
    if (_loading) return;
    auto* p = currentProvider();
    if (!p) return;
    std::string oldName = p->_name;
    std::string newName = _providerNameEdit->text().trimmed().toStdString();
    // 供应商重名拒绝
    if (newName != oldName && isProviderNameDuplicate(newName, _currentProviderIdx)) {
        QMessageBox::warning(this, QStringLiteral("重名"),
                             QStringLiteral("已存在同名供应商，名称未更改"));
        _loading = true;
        _providerNameEdit->setText(QString::fromStdString(oldName));
        _loading = false;
        return;
    }
    // 改名时同步激活引用（当前激活供应商改名 → activeProviderName 跟随）
    if (newName != oldName && oldName == _settings._activeProviderName) {
        _settings._activeProviderName = newName;
    }
    p->_name = newName;
    p->_providerType = static_cast<schema::ProviderType>(_typeCombo->currentData().toInt());
    p->_baseUrl = _baseUrlEdit->text().toStdString();
    p->_apiKey  = _apiKeyEdit->text().toStdString();
    // 名称变化 → 刷新列表标签
    if (oldName != p->_name) {
        _loading = true;
        _providerList->blockSignals(true);
        _providerList->clear();
        for (const auto& pp : _settings._providers) {
            QString label = QString::fromStdString(pp._name);
            if (pp._name == _settings._activeProviderName) label += "  ✓当前";
            _providerList->addItem(label);
        }
        _providerList->blockSignals(false);
        _providerList->setCurrentRow(_currentProviderIdx);
        _loading = false;
    }
}

void SettingsDialog::onAddProvider() {
    schema::LlmProvider p;
    p._providerType = schema::ProviderType::OpenAI;
    std::string base = "新供应商 " + std::to_string(_settings._providers.size() + 1);
    std::string name = base;
    int suffix = 2;
    while (isProviderNameDuplicate(name, -1)) {
        name = base + " (" + std::to_string(suffix) + ")";
        ++suffix;
    }
    p._name = name;
    _settings._providers.push_back(p);
    rebuildProviderList((int)_settings._providers.size() - 1);
}

void SettingsDialog::onDelProvider() {
    int idx = _currentProviderIdx;
    if (idx < 0 || idx >= (int)_settings._providers.size()) return;
    std::string removed = _settings._providers[idx]._name;
    _settings._providers.erase(_settings._providers.begin() + idx);
    if (_settings._activeProviderName == removed) _settings._activeProviderName.clear();
    rebuildProviderList(_settings._providers.empty() ? -1
                       : std::min(idx, (int)_settings._providers.size() - 1));
}

void SettingsDialog::onModelCellChanged(int row, int col) {
    if (_loading) return;
    auto* p = currentProvider();
    if (!p || row < 0 || row >= (int)p->_models.size()) return;
    (void)col;
    std::string oldName = p->_models[row]._name;
    std::string newName = _modelTable->item(row, 0) ? _modelTable->item(row, 0)->text().trimmed().toStdString() : "";
    // 模型重名（同供应商内）拒绝
    if (newName != oldName && !newName.empty() && isModelNameDuplicate(newName, row)) {
        QMessageBox::warning(this, QStringLiteral("重名"),
                             QStringLiteral("该供应商下已存在同名模型，名称未更改"));
        _loading = true;
        _modelTable->item(row, 0)->setText(QString::fromStdString(oldName));
        _loading = false;
        return;
    }
    p->_models[row]._name = newName;
    bool tok = false;
    double t = _modelTable->item(row, 1)->text().trimmed().toDouble(&tok);
    p->_models[row]._temperature = tok ? std::optional<double>(t) : std::nullopt;
    bool mok = false;
    int mn = _modelTable->item(row, 2)->text().trimmed().toInt(&mok);
    p->_models[row]._maxTokens = mok ? std::optional<int>(mn) : std::nullopt;
}

void SettingsDialog::onModelCurrentToggled(int row, bool checked) {
    if (_loading || !checked) return;
    auto* p = currentProvider();
    if (!p || row < 0 || row >= (int)p->_models.size()) return;
    _settings._activeModelName = p->_models[row]._name;
}

void SettingsDialog::onAddModel() {
    auto* p = currentProvider();
    if (!p) return;
    schema::LlmModel m;
    std::string base = "新模型 " + std::to_string(p->_models.size() + 1);
    std::string name = base;
    int suffix = 2;
    // 临时设 _currentProviderIdx 用于 isModelNameDuplicate（currentProvider 已是 p）
    while (isModelNameDuplicate(name, -1)) {
        name = base + " (" + std::to_string(suffix) + ")";
        ++suffix;
    }
    m._name = name;
    p->_models.push_back(m);
    rebuildModelTable();
}

void SettingsDialog::onDelModel() {
    auto* p = currentProvider();
    int row = _modelTable->currentRow();
    if (!p || row < 0 || row >= (int)p->_models.size()) return;
    std::string removed = p->_models[row]._name;
    p->_models.erase(p->_models.begin() + row);
    if (_settings._activeModelName == removed) _settings._activeModelName.clear();
    rebuildModelTable();
}

void SettingsDialog::onPickWorkDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择工作目录",
                 _workDirEdit->text().isEmpty() ? QString() : _workDirEdit->text());
    if (!dir.isEmpty()) _workDirEdit->setText(dir);
}

} // namespace app
} // namespace qh
