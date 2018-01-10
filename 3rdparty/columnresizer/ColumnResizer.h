/*
 * Copyright 2011 Aurélien Gâteau <agateau@kde.org>
 * License: BSD-3-Clause
 */
#ifndef COLUMNRESIZER_H
#define COLUMNRESIZER_H

#include <QtWidgets/QFormLayout>

#include <QtCore/QObject>
#include <QtCore/QList>

class QEvent;
class QGridLayout;
class QLayout;
class QWidget;

class ColumnResizerPrivate;
class ColumnResizer : public QObject
{
    Q_OBJECT
public:
    explicit ColumnResizer(QObject* parent = 0);
    ~ColumnResizer();

    void addWidget(QWidget* widget);
    void addWidgetsFromLayout(QLayout*, int column);
    void addWidgetsFromGridLayout(QGridLayout*, int column);
    void addWidgetsFromFormLayout(QFormLayout*, QFormLayout::ItemRole role);

private Q_SLOTS:
    void updateWidth();

protected:
    bool eventFilter(QObject*, QEvent* event);

private:
    ColumnResizerPrivate* const d;
};

#endif /* COLUMNRESIZER_H */
