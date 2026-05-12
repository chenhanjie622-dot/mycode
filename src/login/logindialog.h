#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QPainter>
#include <QLineEdit>
#include <QCheckBox>
#include "structs/datatypes.h"

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent), m_iconSize(20, 20) {}

    void setLeadingIcon(const QIcon &icon) {
        m_leadingIcon = icon;
        updateTextMargins();
        update();  // 触发重绘
    }

    void setIconSize(const QSize &size) {
        m_iconSize = size;
        updateTextMargins();
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QLineEdit::paintEvent(event);

        if (!m_leadingIcon.isNull()) {
            QPainter painter(this);

            // 绘制图标
            QRect iconRect(15, (height() - m_iconSize.height()) / 2, m_iconSize.width(), m_iconSize.height());
            m_leadingIcon.paint(&painter, iconRect);
        }
    }

private:
    void updateTextMargins() {
        if (!m_leadingIcon.isNull()) {
            int leftMargin = m_iconSize.width() + 30;  // 图标宽度 + 分隔线宽度 + 间距
            setTextMargins(leftMargin, 0, 0, 0);  // 设置文本边距
        } else {
            setTextMargins(0, 0, 0, 0);  // 恢复默认边距
        }
    }

    QIcon m_leadingIcon;
    QSize m_iconSize;
};

namespace Ui {
class LogInDialog;
}

class LogInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogInDialog(QWidget *parent = nullptr);
    ~LogInDialog();

    void InitUI();

protected:
    // 重写 resizeEvent
    void resizeEvent(QResizeEvent *event) override;

private:
    void repositionPasswordCheckBox();

signals:
    void signalLogin();
    void signalLoginSuccess(int roleType);
    void signalLoginUserInfo(const UserInfo& userInfo);

private slots:
    void on_btn_login_clicked();
private:
    Ui::LogInDialog *ui;
    QCheckBox *passwordCheckBox; // 成员变量存储指针
};

#endif // LOGINDIALOG_H
