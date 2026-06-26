#ifndef QH_APP_SETTINGSDIALOG_H
#define QH_APP_SETTINGSDIALOG_H
#include <QDialog>
#include "schema/Settings.h"

class QTabWidget;
class QListWidget;
class QTableWidget;
class QTableWidgetItem;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QButtonGroup;
class QRadioButton;

namespace qh {
namespace app {

// 设置对话框：LLM(供应商列表+模型表格) / 引擎 / 工具 / 工作目录。
// 在 _settings 副本上编辑；确定时由 MainWindow 经 getSettings() 取回并持久化。
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setSettings(const schema::Settings& s);
    schema::Settings getSettings() const;

private slots:
    // 供应商
    void onAddProvider();
    void onDelProvider();
    void onProviderSelected();        // 左侧单击 → 设为当前供应商 + 右侧回显
    void onProviderFieldChanged();    // 供应商表单(名称/类型/url/key)改动 → 回写
    // 模型
    void onAddModel();
    void onDelModel();
    void onModelCellChanged(int row, int col);   // 模型名/temp/maxTokens 编辑 → 回写
    void onModelCurrentToggled(int row, bool checked);  // 当前列 radio → 写 activeModelName
    // 其他
    void onPickWorkDir();

private:
    void rebuildProviderList(int selectRow);
    void loadProviderToForm(int idx);
    void rebuildModelTable();
    int  currentProviderRow() const;

    bool isProviderNameDuplicate(const std::string& name, int exceptIdx) const;
    bool isModelNameDuplicate(const std::string& name, int exceptIdx) const;
    schema::LlmProvider* currentProvider();
    void clearModelTableSignals(bool block);   // 重建表格时屏蔽 cellChanged

    schema::Settings _settings;
    int _currentProviderIdx = -1;
    bool _loading = false;

    QTabWidget*   _tabs = nullptr;
    // 供应商
    QListWidget*  _providerList = nullptr;
    QLineEdit*    _providerNameEdit = nullptr;
    QComboBox*    _typeCombo = nullptr;
    QLineEdit*    _baseUrlEdit = nullptr;
    QLineEdit*    _apiKeyEdit = nullptr;
    // 模型表格
    QTableWidget* _modelTable = nullptr;
    QButtonGroup* _modelRadioGroup = nullptr;   // 当前列 radio 单选
    // 其他 Tab
    QCheckBox*    _thinkingCheck = nullptr;
    QListWidget*  _toolsList = nullptr;
    QLineEdit*    _workDirEdit = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_SETTINGSDIALOG_H
