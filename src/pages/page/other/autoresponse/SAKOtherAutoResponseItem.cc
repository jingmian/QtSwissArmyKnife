﻿/*
 * Copyright 2018-2020 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part
 * of QtSwissArmyKnife project.
 *
 * QtSwissArmyKnife is licensed according to the terms in
 * the file LICENCE in the root of the source code directory.
 */
#include <QDebug>
#include <QDateTime>

#include "SAKGlobal.hh"
#include "SAKDebugPage.hh"
#include "SAKDataStruct.hh"
#include "SAKOtherAutoResponseItem.hh"

#include "ui_SAKOtherAutoResponseItem.h"

SAKOtherAutoResponseItem::SAKOtherAutoResponseItem(SAKDebugPage *debugPage, QWidget *parent)
    :QWidget(parent)
    ,mForbiddenAllAutoResponse(false)
    ,mDebugPage(debugPage)
    ,ui(new Ui::SAKOtherAutoResponseItem)
{
    initUi();
    mID = QDateTime::currentMSecsSinceEpoch();
    remarkLineEdit->setText(QString::number(mID));
    initDelayWritingTimer();
}

SAKOtherAutoResponseItem::SAKOtherAutoResponseItem(SAKDebugPage *debugPage,
                                                   quint64 id,
                                                   QString name,
                                                   QString referenceData,
                                                   QString responseData,
                                                   bool enabled,
                                                   quint32 referenceFormat,
                                                   quint32 responseFormat,
                                                   quint32 option,
                                                   QWidget *parent)
    :QWidget(parent)
    ,mForbiddenAllAutoResponse(false)
    ,mDebugPage(debugPage)
    ,mID(id)
    ,ui(new Ui::SAKOtherAutoResponseItem)
{
    initUi();
    remarkLineEdit->setText(name);
    referenceLineEdit->setText(referenceData);
    responseLineEdit->setText(responseData);
    enableCheckBox->setChecked(enabled);
    referenceDataFromatComboBox->setCurrentIndex(referenceFormat);
    responseDataFormatComboBox->setCurrentIndex(responseFormat);
    optionComboBox->setCurrentIndex(option);
    initDelayWritingTimer();
}

SAKOtherAutoResponseItem::~SAKOtherAutoResponseItem()
{
    delete ui;
}

void SAKOtherAutoResponseItem::setAllAutoResponseDisable(bool disable)
{
    mForbiddenAllAutoResponse = disable;
}

quint64 SAKOtherAutoResponseItem::itemID()
{
    return mID;
}

QString SAKOtherAutoResponseItem::itemDescription()
{
    return remarkLineEdit->text();
}

QString SAKOtherAutoResponseItem::itemRefernceText()
{
    return referenceLineEdit->text();
}

QString SAKOtherAutoResponseItem::itemResponseText()
{
    return responseLineEdit->text();
}

bool SAKOtherAutoResponseItem::itemEnable()
{
    return enableCheckBox->isChecked();
}

quint32 SAKOtherAutoResponseItem::itemReferenceFormat()
{
    return referenceDataFromatComboBox->currentIndex();
}

quint32 SAKOtherAutoResponseItem::itemResponseFormat()
{
    return responseDataFormatComboBox->currentIndex();
}

quint32 SAKOtherAutoResponseItem::itemOption()
{
    return optionComboBox->currentIndex();
}

void SAKOtherAutoResponseItem::setLineEditFormat(QLineEdit *lineEdit, int format)
{
    QRegExp regExpBin("([01][01][01][01][01][01][01][01][ ])*");
    QRegExp regExpOct("([0-7][0-7][ ])*");
    QRegExp regExpDec("([0-9][0-9][ ])*");
    QRegExp regExpHex("([0-9A-F][0-9A-F][ ])*");
    QRegExp regExpAscii("([a-zA-Z0-9`~!@#$%^&*()-_=+\\|;:'\",<.>/? ])*");

    if (lineEdit){
        lineEdit->setValidator(Q_NULLPTR);
        lineEdit->clear();
        switch (format) {
        case SAKDataStruct::InputFormatBin:
            lineEdit->setValidator(new QRegExpValidator(regExpBin, this));
            break;
        case SAKDataStruct::InputFormatOct:
            lineEdit->setValidator(new QRegExpValidator(regExpOct, this));
            break;
        case SAKDataStruct::InputFormatDec:
            lineEdit->setValidator(new QRegExpValidator(regExpDec, this));
            break;
        case SAKDataStruct::InputFormatHex:
            lineEdit->setValidator(new QRegExpValidator(regExpHex, this));
            break;
        case SAKDataStruct::InputFormatAscii:
            lineEdit->setValidator(new QRegExpValidator(regExpAscii, this));
            break;
        case SAKDataStruct::InputFormatLocal:
            lineEdit->setValidator(Q_NULLPTR);
            break;
        default:
            break;
        }
    }
}

void SAKOtherAutoResponseItem::bytesRead(QByteArray bytes)
{
    if (mForbiddenAllAutoResponse){
        return;
    }

    if (bytes.isEmpty()){
        return;
    }

    if (!enableCheckBox->isChecked()){
        return;
    }

    /// @brief 判断是否回复
    QString referenceString = referenceLineEdit->text();
    int referenceFormat = referenceDataFromatComboBox->currentData().toInt();
    QByteArray referenceData = string2array(referenceString, referenceFormat);
    if (response(bytes, referenceData, optionComboBox->currentData().toInt())){
         QString responseString = responseLineEdit->text();
         int responseFromat = responseDataFormatComboBox->currentData().toInt();
         QByteArray responseData = string2array(responseString, responseFromat);

         if (!responseData.isEmpty()){
             /// @brief 延时回复
             if (delayResponseCheckBox->isChecked()){
                quint32 delayTime = delayResponseLineEdit->text().toUInt();
                if (delayTime < 40){
                    delayTime = 20;
                }

                /// @brief 延时回复
                DelayWritingInfo *info = new DelayWritingInfo;
                /// @brief 减20是因为延时回复使用20毫秒的定时器
                info->expectedTimestamp = QDateTime::currentMSecsSinceEpoch() + delayTime - 20;
                info->data = responseData;
                mWaitForWrittenInfoList.append(info);
             }else{
                 mDebugPage->write(responseData);
             }
         }
    }
}

QByteArray SAKOtherAutoResponseItem::string2array(QString str, int format)
{
    auto stringList2Array = [](QStringList strList, int base) -> QByteArray{
        QByteArray array;
        for (auto var:strList){
            array.append(static_cast<char>(var.toInt(Q_NULLPTR, base)));
        }
        return array;
    };

    QByteArray array;
    QStringList strList;
    int base;
    switch (format) {
    case SAKDataStruct::InputFormatBin:
        base = 2;
        strList = str.split(' ');
        array = stringList2Array(strList, base);
        break;
    case SAKDataStruct::InputFormatOct:
        base = 8;
        strList = str.split(' ');
        array = stringList2Array(strList, base);
        break;
    case SAKDataStruct::InputFormatDec:
        base = 10;
        strList = str.split(' ');
        array = stringList2Array(strList, base);
        break;
    case SAKDataStruct::InputFormatHex:
        base = 16;
        strList = str.split(' ');
        array = stringList2Array(strList, base);
        break;
    case SAKDataStruct::InputFormatAscii:
        array = str.toLatin1();
        break;
    case SAKDataStruct::InputFormatLocal:
        array = str.toLocal8Bit();
        break;
    default:
        array = str.toLatin1();
    }

    return array;
};

bool SAKOtherAutoResponseItem::response(QByteArray receiveData, QByteArray referenceData, int option)
{
    if (option == SAKDataStruct::AutoResponseOptionEqual){
        return (QString(receiveData.toHex()).compare(QString(referenceData.toHex())) == 0);
    }

    if (option == SAKDataStruct::AutoResponseOptionContain){
        return (QString(receiveData.toHex()).contains(QString(referenceData.toHex())));
    }

    if (option == SAKDataStruct::AutoResponseOptionDoNotContain){
        return !(QString(receiveData.toHex()).contains(QString(referenceData.toHex())));
    }

    return false;
};

void SAKOtherAutoResponseItem::initUi()
{
    ui->setupUi(this);
    remarkLineEdit = ui->remarkLineEdit;
    referenceLineEdit = ui->referenceLineEdit;
    responseLineEdit = ui->responseLineEdit;
    enableCheckBox = ui->enableCheckBox;
    optionComboBox = ui->optionComboBox;
    updatePushButton = ui->updatePushButton;
    referenceDataFromatComboBox = ui->referenceDataFromatComboBox;
    responseDataFormatComboBox  = ui->responseDataFormatComboBox;
    delayResponseCheckBox = ui->delayResponseCheckBox;
    delayResponseLineEdit = ui->delayResponseLineEdit;

    optionComboBox->clear();
    optionComboBox->addItem(tr("接收数据等于参考数据时自动回复"), QVariant::fromValue<int>(SAKDataStruct::AutoResponseOptionEqual));
    optionComboBox->addItem(tr("接收数据包含参考数据时自动回复"), QVariant::fromValue<int>(SAKDataStruct::AutoResponseOptionContain));
    optionComboBox->addItem(tr("接收数据不包含参考数据时自动回复"), QVariant::fromValue<int>(SAKDataStruct::AutoResponseOptionDoNotContain));

    SAKGlobal::initInputTextFormatComboBox(referenceDataFromatComboBox);
    SAKGlobal::initInputTextFormatComboBox(responseDataFormatComboBox);

    connect(mDebugPage, &SAKDebugPage::bytesRead, this, &SAKOtherAutoResponseItem::bytesRead);
}

void SAKOtherAutoResponseItem::initDelayWritingTimer()
{
    mTimestampChecker.setInterval(20);
    connect(&mTimestampChecker, &QTimer::timeout, this, &SAKOtherAutoResponseItem::delayToWritBytes);
    mTimestampChecker.start();
}

void SAKOtherAutoResponseItem::delayToWritBytes()
{
    mTimestampChecker.stop();
    QList<DelayWritingInfo> temp;
    QList<DelayWritingInfo*> need2removeList;
    for (int i = 0; i < mWaitForWrittenInfoList.length(); i++){
        DelayWritingInfo *infoPtr = mWaitForWrittenInfoList.at(i);
        if (quint64(QDateTime::currentMSecsSinceEpoch()) > infoPtr->expectedTimestamp){
            DelayWritingInfo info;
            info.data = infoPtr->data;
            info.expectedTimestamp = infoPtr->expectedTimestamp;
            temp.append(info);
            need2removeList.append(infoPtr);
        }
    }

    /// @brief 发送数据
    for (auto var : temp){
        mDebugPage->write(var.data);
    }

    /// @brief 删除已发送的数据
    for (auto var : need2removeList){
        mWaitForWrittenInfoList.removeOne(var);
    }
    mTimestampChecker.start();
}

void SAKOtherAutoResponseItem::on_referenceDataFromatComboBox_currentTextChanged()
{
    setLineEditFormat(referenceLineEdit, referenceDataFromatComboBox->currentData().toInt());
}

void SAKOtherAutoResponseItem::on_responseDataFormatComboBox_currentTextChanged()
{
    setLineEditFormat(responseLineEdit, responseDataFormatComboBox->currentData().toInt());
}

void SAKOtherAutoResponseItem::on_updatePushButton_clicked()
{
    QString tableName = SAKDataStruct::autoResponseTableName(mDebugPage->pageType());
    SAKDataStruct::SAKStructAutoResponseItem item;
    item.id = itemID();
    item.name = itemDescription();
    item.enable = itemEnable();
    item.responseData = itemResponseText();
    item.referenceData = itemRefernceText();
    item.responseFormat = itemResponseFormat();
    item.referenceFormat = itemReferenceFormat();
    item.option = itemOption();
//    SAKDebugPageDatabaseInterface::instance()->updateAutoResponseItem(tableName, item);
}
