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

// 可在设置窗口勾选加载的工具清单（单一数据源，驱动工具 Tab 的构建/回显/收集；
// 新增工具只需在此登记，并在 EngineThread 按名注册对应实现）
static const char* const kToolNames[] = { "bash", "read_file", "write_file", "edit_file" };

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
    engV->addWidget(new QLabel("引擎模式："));
    _reactRadio    = new QRadioButton("普通 ReAct");
    _twoStageRadio = new QRadioButton("两阶段 ReAct（慢思考）");
    _reactRadio->setChecked(true);   // 默认普通 ReAct（与 _enableThinking 默认 false 一致；setSettings 会覆盖）
    engV->addWidget(_reactRadio);
    engV->addWidget(_twoStageRadio);
    engV->addStretch();
    _tabs->addTab(eng, "引擎");

    // ===== Tab 3: 工具 =====
    auto* tools = new QWidget;
    auto* toolsV = new QVBoxLayout(tools);
    _toolsList = new QListWidget;
    for (const char* toolName : kToolNames) {
        _toolsList->addItem(toolName);
        QListWidgetItem* it = _toolsList->item(_toolsList->count() - 1);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setCheckState(Qt::Unchecked);
    }
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
    (s._enableThinking ? _twoStageRadio : _reactRadio)->setChecked(true);
    _workDirEdit->setText(QString::fromStdString(s._workDir));
    // 按名回显各工具勾选态（列表行序与 kToolNames 一致）
    for (int i = 0; i < _toolsList->count(); ++i) {
        QListWidgetItem* item = _toolsList->item(i);
        std::string toolName = item->text().toStdString();
        bool on = std::find(s._enabledTools.begin(), s._enabledTools.end(), toolName)
                  != s._enabledTools.end();
        item->setCheckState(on ? Qt::Checked : Qt::Unchecked);
    }
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
    s._enableThinking = _twoStageRadio->isChecked();
    s._workDir = _workDirEdit->text().trimmed().toStdString();
    s._enabledTools.clear();
    // 收集所有勾选的工具名
    for (int i = 0; i < _toolsList->count(); ++i) {
        QListWidgetItem* item = _toolsList->item(i);
        if (item && item->checkState() == Qt::Checked) {
            s._enabledTools.push_back(item->text().toStdString());
        }
    }
    return s;
}

int SettingsDialog::currentProviderRow() const { return _providerList->currentRow(); }

schema::LlmProvider* SettingsDialog::currentProvider() {
    int idx = _currentProviderIdx;
    if (idx < 0 || idx >= (int)_settings._providers.size()) return nullptr;
    return &_settings._providers[idx];
}

const schema::LlmProvider* SettingsDialog::currentProvider() const {
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
    const schema::LlmProvider* p = currentProvider();   // const 重载，无需 const_cast
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
    if (selectRow < 0 || selectRow >= _providerList->count()) selectRow = -1;
    _loading = true;
    if (selectRow >= 0) _providerList->setCurrentRow(selectRow);   // 屏蔽 onProviderSelected
    _loading = false;
    loadProviderToForm(selectRow);   // 显式回显（不改 _activeProviderName）
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
    // 清旧 radio：只从 group 移除，**不要 delete**——radio 是 QTableWidget 的 cellWidget，
    // 所有权归 table；手动 delete 会让 table 内部悬挂指针，setRowCount 时崩溃（double free）。
    // 由 setRowCount(0) 释放旧 cellWidget（此时 radio 已从 group 移除，group 安全）。
    for (auto* b : _modelRadioGroup->buttons()) {
        _modelRadioGroup->removeButton(b);
    }
    auto* p = currentProvider();
    int rows = p ? (int)p->_models.size() : 0;
    _modelTable->setRowCount(0);   // 清所有行 + 释放旧 cellWidget（radio）
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
    // M1: 空模型名拒绝并回滚
    if (newName.empty()) {
        QMessageBox::warning(this, QStringLiteral("无效"),
                             QStringLiteral("模型名不能为空"));
        _loading = true;
        _modelTable->item(row, 0)->setText(QString::fromStdString(oldName));
        _loading = false;
        return;
    }
    // 模型重名（同供应商内）拒绝
    if (newName != oldName && isModelNameDuplicate(newName, row)) {
        QMessageBox::warning(this, QStringLiteral("重名"),
                             QStringLiteral("该供应商下已存在同名模型，名称未更改"));
        _loading = true;
        _modelTable->item(row, 0)->setText(QString::fromStdString(oldName));
        _loading = false;
        return;
    }
    // I1: 改名时同步激活引用（当前激活模型改名 → activeModelName 跟随）
    if (newName != oldName && oldName == _settings._activeModelName) {
        _settings._activeModelName = newName;
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
    _settings._activeProviderName = p->_name;   // 选模型隐含选其供应商（双激活一致；否则未单击供应商直接选 radio 时 _activeProviderName 为空，再打开 radio 回显丢失）
}

void SettingsDialog::onAddModel() {
    auto* p = currentProvider();
    if (!p) return;
    schema::LlmModel m;
    std::string base = "新模型 " + std::to_string(p->_models.size() + 1);
    std::string name = base;
    int suffix = 2;
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
