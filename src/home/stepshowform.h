#ifndef STEPSHOWFORM_H
#define STEPSHOWFORM_H

#include "qboxlayout.h"
#include "rackcfgform.h"
#include <QWidget>

namespace Ui {
class StepShowForm;
}

class StepShowForm : public QWidget
{
    Q_OBJECT

public:
    explicit StepShowForm(QWidget *parent = nullptr);
    ~StepShowForm();

    void setStepValue(QString value);
    void setImageValue(QString value);
    void setRackDataValue(QString value);

    QString getStepText() const;
    QString getImagePath() const;
    void setProjectorVideoPath(QString path);
    QString getProjectorVideoPath() const;
    QVector<QSharedPointer<RackCFGForm>> getRackForm() const;

    bool isChecked() const;
    void setChecked(bool checked);

    void setVideoPath(QString path);
    QString getVideoPath() const;

    void setProjectorImagePath(QString path);
    QString getProjectorImagePath() const;

    void setSopText(QString text);
    QString getSopText() const;

signals:
    void signalDelete(StepShowForm* self);
    void signalEdit(StepShowForm* self);
    void signalMoveUp(StepShowForm* self);
    void signalMoveDown(StepShowForm* self);

private:
    bool isRackDataValid(const QStringList& rackData);
    void createAndAddRackForm(QList<QSharedPointer<RackCFGForm>>& rackForm, QHBoxLayout *layout,
                              const QString& rackNumber = "", const QString& quantityGoods = "");
private:
    Ui::StepShowForm *ui;
    QString m_videoPath;
    QString m_stepText;
    QString m_imagePath;
    QString m_projectorImagePath; // 专门存投影仪要放的图片
    QStringList m_rackdata;
    QVector<QSharedPointer<RackCFGForm>> m_rackForm;
    QString m_sopText;
    QString m_projectorVideoPath; // 专门存投影仪要放的视频
};

#endif
