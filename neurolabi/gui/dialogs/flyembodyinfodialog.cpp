#include <iostream>

#include <QtGui>
#include <QMessageBox>

#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#else
#include <QtCore>
#endif

#include "zjsonarray.h"
#include "zjsonobject.h"
#include "zjsonparser.h"
#include "zstring.h"

#include "dvid/zdvidtarget.h"
#include "dvid/zdvidreader.h"

#include "flyembodyinfodialog.h"
#include "ui_flyembodyinfodialog.h"
#include "zdialogfactory.h"
#include "zstring.h"

/*
 * this dialog displays a list of bodies and their properties; data is
 * loaded from a static json bookmarks file
 *
 * to add/remove/alter columns in table:
 * -- in createModel(), change ncol
 * -- in setHeaders(), adjust headers
 * -- in updateModel(), adjust data load and initial sort order
 *
 * I really should be creating model and headers from a constant headers list
 *
 * djo, 7/15
 *
 */
FlyEmBodyInfoDialog::FlyEmBodyInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlyEmBodyInfoDialog)
{
    ui->setupUi(this);

    init();

    m_model = createModel(ui->tableView);

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // -1 = filter on all columns!  ALL!  no column left behind!
    m_proxy->setFilterKeyColumn(-1);
    ui->tableView->setModel(m_proxy);

    // UI connects
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(onCloseButton()));
    connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(onRefreshButton()));
    connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)),
        this, SLOT(activateBody(QModelIndex)));
    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterUpdated(QString)));
    connect(ui->clearFilterButton, SIGNAL(clicked(bool)), ui->filterLineEdit, SLOT(clear()));
    connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(applicationQuitting()));

    // data update connects
    // register our type so we can signal/slot it across threads:
    qRegisterMetaType<ZJsonValue>("ZJsonValue");
    connect(this, SIGNAL(dataChanged(ZJsonValue)), this, SLOT(updateModel(ZJsonValue)));
    connect(this, SIGNAL(loadCompleted()), this, SLOT(updateStatusAfterLoading()));
    connect(this, SIGNAL(jsonLoadError(QString)), this, SLOT(onJsonLoadError(QString)));

}

void FlyEmBodyInfoDialog::init()
{
    m_quitting = false;
}

void FlyEmBodyInfoDialog::activateBody(QModelIndex modelIndex)
{
  if (modelIndex.column() == 0) {
    QStandardItem *item = m_model->itemFromIndex(m_proxy->mapToSource(modelIndex));
    uint64_t bodyId = item->data(Qt::DisplayRole).toULongLong();

#ifdef _DEBUG_
    std::cout << bodyId << " activated." << std::endl;
#endif

    emit bodyActivated(bodyId);
  }
}

void FlyEmBodyInfoDialog::dvidTargetChanged(ZDvidTarget target) {
#ifdef _DEBUG_
    std::cout << "dvid target changed to " << target.getUuid() << std::endl;
#endif

    // store dvid target (may not be necessary, now that I removed the
    //  option to turn off autoload?)
    m_currentDvidTarget = target;

    // if target isn't null, trigger load in thread
    if (target.isValid()) {
        // clear the model regardless at this point
        m_model->clear();
        setStatusLabel("Loading...");

        // we can load this info from different sources, depending on
        //  what's available in DVID

        // is the synapse file present?
        if (dvidBookmarksPresent(target)) {
            QtConcurrent::run(this, &FlyEmBodyInfoDialog::importBookmarksDvid, target);
        } else if (bodies3Present(target)) {
            // this is the current expected fallback method...
            QtConcurrent::run(this, &FlyEmBodyInfoDialog::importBodiesDvid, target);
        } else {
            // ...but sometimes, we've got nothing
            emit loadCompleted();
        }
    }
}

void FlyEmBodyInfoDialog::applicationQuitting() {
    m_quitting = true;
}

QStandardItemModel* FlyEmBodyInfoDialog::createModel(QObject* parent) {
    QStandardItemModel* model = new QStandardItemModel(0, 5, parent);
    setHeaders(model);
    return model;
}

void FlyEmBodyInfoDialog::setHeaders(QStandardItemModel * model) {
    model->setHorizontalHeaderItem(0, new QStandardItem(QString("Body ID")));
    model->setHorizontalHeaderItem(1, new QStandardItem(QString("name")));
    model->setHorizontalHeaderItem(2, new QStandardItem(QString("# pre")));
    model->setHorizontalHeaderItem(3, new QStandardItem(QString("# post")));
    model->setHorizontalHeaderItem(4, new QStandardItem(QString("status")));
}

void FlyEmBodyInfoDialog::setStatusLabel(QString label) {
    ui->statusLabel->setText(label);
}

void FlyEmBodyInfoDialog::clearStatusLabel() {
    ui->statusLabel->setText("");
}

void FlyEmBodyInfoDialog::updateStatusLabel() {
    qlonglong nPre = 0;
    qlonglong nPost = 0;
    for (qlonglong i=0; i<m_proxy->rowCount(); i++) {
        nPre += m_proxy->data(m_proxy->index(i, 2)).toLongLong();
        nPost += m_proxy->data(m_proxy->index(i, 3)).toLongLong();
    }

    // have I mentioned how much I despise C++ strings?
    std::ostringstream outputStream;
    outputStream << "Showing " << m_proxy->rowCount() << "/" << m_model->rowCount() << " bodies, ";
    outputStream << nPre << "/" <<  m_totalPre << " pre-syn, ";
    outputStream << nPost << "/" <<  m_totalPost << " post-syn";
    setStatusLabel(QString::fromStdString(outputStream.str()));
}

void FlyEmBodyInfoDialog::updateStatusAfterLoading() {
    if (m_model->rowCount() > 0) {
        updateStatusLabel();
    } else {
        clearStatusLabel();
    }
}

void FlyEmBodyInfoDialog::filterUpdated(QString filterText) {
    m_proxy->setFilterFixedString(filterText);

    // turns out you need to explicitly tell it to resort after the filter
    //  changes; if you don't, and new filter shows more items, those items
    //  will appear somewhere lower than the existing items
    m_proxy->sort(m_proxy->sortColumn(), m_proxy->sortOrder());
    updateStatusLabel();
}

bool FlyEmBodyInfoDialog::dvidBookmarksPresent(ZDvidTarget target) {
    ZDvidReader reader;
    reader.setVerbose(false);
    if (reader.open(target)) {
        // check for data name and key
        if (!reader.hasData(ZDvidData::GetName(ZDvidData::ROLE_BODY_ANNOTATION))) {
            #ifdef _DEBUG_
                std::cout << "UUID doesn't have body annotations" << std::endl;
            #endif
            return false;
        }

        // I don't like this hack, but we seem not to have "hasKey()", or any way to detect
        //  a failure to find a key (the reader doesn't report, eg, 404s after a call)
        if (reader.readKeys(ZDvidData::GetName(ZDvidData::ROLE_BODY_ANNOTATION),
            ZDvidData::GetName(ZDvidData::ROLE_BODY_SYNAPSES), 
            ZDvidData::GetName(ZDvidData::ROLE_BODY_SYNAPSES)).size() == 0) {
            #ifdef _DEBUG_
                std::cout << "UUID doesn't have body_synapses key" << std::endl;
            #endif
            return false;
        }
        return true;
    } else {
        return false;
    }
}

bool FlyEmBodyInfoDialog::bodies3Present(ZDvidTarget target) {
    ZDvidReader reader;
    reader.setVerbose(false);
    if (reader.open(target)) {
        // check for data name and key
        if (!reader.hasData(ZDvidData::GetName(ZDvidData::ROLE_BODY_ANNOTATION,
            ZDvidData::ROLE_BODY_LABEL, target.getBodyLabelName()))) {
            #ifdef _DEBUG_
                std::cout << "UUID doesn't have bodies3 annotations" << std::endl;
            #endif
            return false;
        }
        return true;
    } else {
        return false;
    }
}

void FlyEmBodyInfoDialog::importBookmarksDvid(ZDvidTarget target) {
#ifdef _DEBUG_
    std::cout << "loading bookmarks from " << target.getUuid() << std::endl;
#endif

    // this method assumes DVID target is valid, and necessary DVID keys
    //  are present

    ZDvidReader reader;
    reader.setVerbose(false);
    if (reader.open(target)) {
        const QByteArray &bookmarkData = reader.readKeyValue(
            ZDvidData::GetName(ZDvidData::ROLE_BODY_ANNOTATION),
            ZDvidData::GetName(ZDvidData::ROLE_BODY_SYNAPSES));
        ZJsonObject jsonDataObject;
        jsonDataObject.decodeString(bookmarkData.data());

        // validate; this method does its own error notifications
        if (!isValidBookmarkFile(jsonDataObject)) {
            emit loadCompleted();
            return;
        }

        // now we try to fill in name and status data from more dvid calls
        // note: not sure how well this will scale, as we're querying
        //  every body's data individually
        ZJsonArray bookmarks(jsonDataObject.value("data"));
        #ifdef _DEBUG_
            std::cout << "number of bookmarks to process = " << bookmarks.size() << std::endl;
        #endif

        QString bodyAnnotationName = ZDvidData::GetName(
              ZDvidData::ROLE_BODY_ANNOTATION,
              ZDvidData::ROLE_BODY_LABEL,
              target.getBodyLabelName()).c_str();

        // get all the keys rather than testing whether each body ID
        //  has a name individually
        QStringList keyList = reader.readKeys(bodyAnnotationName);
        QSet<uint64_t> bodySet;
        foreach (const QString &str, keyList) {
          std::vector<uint64_t> bodyIdArray =
              ZString(str.toStdString()).toUint64Array();
          if (bodyIdArray.size() == 1) {
            uint64_t bodyId = bodyIdArray.front();
            if (bodyId > 0) {
              bodySet.insert(bodyId);
            }
          }
        }

        for (size_t i = 0; i < bookmarks.size(); ++i) {
            // if application is quitting, return = exit thread
            if (m_quitting) {
                return;
            }

            ZJsonObject bkmk(bookmarks.at(i), false);

            uint64_t bodyId = bkmk.value("body ID").toInteger();
            if (bodySet.contains(bodyId)) {
                const QByteArray &temp = reader.readKeyValue(bodyAnnotationName,
                      QString::number(bkmk.value("body ID").toInteger()));
                ZJsonObject tempJson;
                tempJson.decodeString(temp.data());

                #ifdef _DEBUG_
                    std::cout << "parsing info for body ID = " << bkmk.value("body ID").toInteger() << std::endl;
                    std::cout << "name = " << ZJsonParser::stringValue(tempJson["name"]) << std::endl;
                    std::cout << "status = " << ZJsonParser::stringValue(tempJson["status"]) << std::endl;
                #endif

                // now push the value back in; don't put empty strings in (messes with sorting)
                // updateModel expects "body status", not "status" (matches original file version)
                if (tempJson.hasKey("status") && strlen(ZJsonParser::stringValue(tempJson["status"])) > 0) {
                    bkmk.setEntry("body status", tempJson["status"]);
                }
                if (tempJson.hasKey("name") && strlen(ZJsonParser::stringValue(tempJson["name"])) > 0) {
                    bkmk.setEntry("name", tempJson["name"]);
                    }
                }
            }

        // no "loadCompleted()" here; it's emitted in updateModel(), when it's done
        emit dataChanged(jsonDataObject.value("data"));
    } else {
        // but we need to clear the loading message if we can't read from DVID
        emit loadCompleted();
    }
}

bool FlyEmBodyInfoDialog::isValidBookmarkFile(ZJsonObject jsonObject) {
    // validation is admittedly limited for now; ultimately, I
    //  expect we'll be getting the info from DVID rather than
    //  a file anyway, so I'm not going to spend much time on it
    
    // in a perfect world, all these json constants would be collected
    //  somewhere central

    if (!jsonObject.hasKey("data") || !jsonObject.hasKey("metadata")) {
        emit jsonLoadError("This file is missing 'data' or 'metadata'. Are you sure this is a Fly EM JSON file?");
        return false;
    }

    ZJsonObject metadata = (ZJsonObject) jsonObject.value("metadata");
    if (!metadata.hasKey("description")) {
        emit jsonLoadError("This file is missing 'metadata/description'. Are you sure this is a Fly EM JSON file?");
        return false;
    }

    ZString description = ZJsonParser::stringValue(metadata["description"]);
    if (description != "bookmarks") {
        emit jsonLoadError("This json file does not have description 'bookmarks'!");
        return false;
    }

    ZJsonValue data = jsonObject.value("data");
    if (!data.isArray()) {
        emit jsonLoadError("The data section in this json file is not an array!");
        return false;
    }

    // we could/should test all the elements of the array to see if they
    //  are bookmarks, but enough is enough...

    return true;
}

void FlyEmBodyInfoDialog::importBodiesDvid(ZDvidTarget target) {
    // this method is a lot like importBookmarksDvid; where that method
    //  (1) reads a json file from dvid, then (2) adds name,= & status 
    //  from different DVID call, this one just goes straight to step 
    //  two; as a result, a lot of the code is copied from there 
    //  (and should probably be refactored to remove duplication)

    // DVID target assumed to be valid 

    ZDvidReader reader;
    reader.setVerbose(false);
    if (reader.open(target)) {
        // we need to construct a json structure from scratch to
        //  match what we'd get out of the bookmarks annotation file;
        //  probably this should be refactored 

        // get all the bodies that have annotations in this UUID        
        // note that this list contains body IDs in strings, *plus*
        //  some other nonnumeric strings (!!)

        QString bodyAnnotationName = ZDvidData::GetName(
              ZDvidData::ROLE_BODY_ANNOTATION,
              ZDvidData::ROLE_BODY_LABEL,
              target.getBodyLabelName()).c_str();
        QStringList keyList = reader.readKeys(bodyAnnotationName);

        ZJsonArray bodies;
        bool ok;
        qlonglong bodyID;
        foreach (const QString &bodyIDstr, keyList) {
            // skip the few non-numeric keys mixed in there
            bodyID = bodyIDstr.toLongLong(&ok);
            if (ok) {
                // get body annotations and transform to what we need
                const QByteArray &temp = reader.readKeyValue(bodyAnnotationName, bodyIDstr);
                ZJsonObject bodyData;
                bodyData.decodeString(temp.data());

                // remove name if empty
                if (bodyData.hasKey("name") && strlen(ZJsonParser::stringValue(bodyData["name"])) == 0) {
                    bodyData.removeKey("name");
                }

                // remove status if empty; change status > body status
                if (bodyData.hasKey("status")) {
                    if (strlen(ZJsonParser::stringValue(bodyData["status"])) > 0) {
                        bodyData.setEntry("body status", bodyData["status"]);
                    } 
                    // don't really need to remove this, but why not
                    bodyData.removeKey("status");
                }

                bodies.append(bodyData);
            } 
        }

        // no "loadCompleted()" here; it's emitted in updateModel(), when it's done
        emit dataChanged(bodies);
    } else {
        // but we need to clear the loading message if we can't read from DVID
        emit loadCompleted();
    }
}

void FlyEmBodyInfoDialog::onRefreshButton() {
    if (m_currentDvidTarget.isValid()) {
        ui->filterLineEdit->clear();
        dvidTargetChanged(m_currentDvidTarget);
    }
}

void FlyEmBodyInfoDialog::onCloseButton() {
    close();
}

/*
 * update the data model from json; input should be the
 * "data" part of our standard bookmarks json file
 *
 * input should be json array of dictionaries looking
 * like this (all values basically optional except 
 * body ID, extras ignored):
 * 
 *    {
 *      "name": "neuron name",
 *      "body status": "Not examined",
 *      "body PSDs": 0,
 *      "body T-bars": 1,
 *      "body ID": 15379594
 *    }
 */
void FlyEmBodyInfoDialog::updateModel(ZJsonValue data) {
    m_model->clear();
    setHeaders(m_model);

    m_totalPre = 0;
    m_totalPost = 0;

    ZJsonArray bookmarks(data);
    m_model->setRowCount(bookmarks.size());
    for (size_t i = 0; i < bookmarks.size(); ++i) {
        ZJsonObject bkmk(bookmarks.at(i), false);

        // carefully set data for column items so they will sort
        //  properly (eg, IDs numerically, not lexically)
        qulonglong bodyID = ZJsonParser::integerValue(bkmk["body ID"]);
        QStandardItem * bodyIDItem = new QStandardItem();
        bodyIDItem->setData(QVariant(bodyID), Qt::DisplayRole);
        m_model->setItem(i, 0, bodyIDItem);

        if (bkmk.hasKey("name")) {
            const char* name = ZJsonParser::stringValue(bkmk["name"]);
            m_model->setItem(i, 1, new QStandardItem(QString(name)));
        }

        if (bkmk.hasKey("body T-bars")) {
            int nPre = ZJsonParser::integerValue(bkmk["body T-bars"]);
            m_totalPre += nPre;
            QStandardItem * preSynapseItem = new QStandardItem();
            preSynapseItem->setData(QVariant(nPre), Qt::DisplayRole);
            m_model->setItem(i, 2, preSynapseItem);
        }

        if (bkmk.hasKey("body PSDs")) {
            int nPost = ZJsonParser::integerValue(bkmk["body PSDs"]);
            m_totalPost += nPost;
            QStandardItem * postSynapseItem = new QStandardItem();
            postSynapseItem->setData(QVariant(nPost), Qt::DisplayRole);
            m_model->setItem(i, 3, postSynapseItem);
        }

        // note that this routine expects "body status", not "status";
        //  historical side-effect of the original file format we read from
        if (bkmk.hasKey("body status")) {
            const char* status = ZJsonParser::stringValue(bkmk["body status"]);
            m_model->setItem(i, 4, new QStandardItem(QString(status)));
        }
    }
    // the resize isn't reliable, so set the name column wider by hand
    ui->tableView->resizeColumnsToContents();
    ui->tableView->setColumnWidth(1, 150);

    // currently initially sorting on # pre-synaptic sites
    ui->tableView->sortByColumn(2, Qt::DescendingOrder);

    emit loadCompleted();
}

void FlyEmBodyInfoDialog::onJsonLoadError(QString message) {
    QMessageBox errorBox;
    errorBox.setText("Error loading body information");
    errorBox.setInformativeText(message);
    errorBox.setStandardButtons(QMessageBox::Ok);
    errorBox.setIcon(QMessageBox::Warning);
    errorBox.exec();
}

FlyEmBodyInfoDialog::~FlyEmBodyInfoDialog()
{
    delete ui;
}
