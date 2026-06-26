#ifndef QH_APP_SETTINGSDIALOG_H
#define QH_APP_SETTINGSDIALOG_H
#include <QDialog>
#include "schema/Settings.h"

class QTabWidget;
class QListWidget;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QPushButton;

namespace qh {
namespace app {

// 设置对话框：单 QTabWidget 四个 Tab（LLM 多 profile / 引擎 / 工具 / 工作目录）。
// 在 _settings 副本上编辑；确定时由 MainWindow 经 getSettings() 取回并持久化。
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setSettings(const schema::Settings& s);
    schema::Settings getSettings() const;

private slots:
    void onAddProfile();
    void onDelProfile();
    void onProfileSelected();     // 列表选中切换 → 表单回显
    void onEditChanged();         // 表单任意字段改动 → 写回当前 profile
    void onSetActive();           // 把当前选中 profile 设为激活
    void onPickWorkDir();

private:
    void rebuildProfileList(int selectRow);
    void loadProfileToForm(int idx);
    int  currentRow() const;

    schema::Settings _settings;
    bool _loading = false;        // 回显时屏蔽 onEditChanged，避免回写循环

    QTabWidget*  _tabs = nullptr;
    QListWidget* _profileList = nullptr;
    QLineEdit*   _nameEdit = nullptr;
    QComboBox*   _typeCombo = nullptr;
    QLineEdit*   _baseUrlEdit = nullptr;
    QLineEdit*   _apiKeyEdit = nullptr;
    QLineEdit*   _modelEdit = nullptr;
    QLineEdit*   _tempEdit = nullptr;
    QLineEdit*   _maxTokensEdit = nullptr;
    QCheckBox*   _thinkingCheck = nullptr;
    QListWidget* _toolsList = nullptr;
    QLineEdit*   _workDirEdit = nullptr;
};

} // namespace app
} // namespace qh
#endif // QH_APP_SETTINGSDIALOG_H
