#include "assetwindow.h"

AssetWindow::AssetWindow(Game* g) :
    game(g),
    update(true)
{

//    table_widget.setSelectionMode(QAbstractItemView::SingleSelection);
//    table_widget.setSelectionMode(QAbstractItemView::ExtendedSelection);
//    table_widget.setDragEnabled(true);
    table_widget.viewport()->setAcceptDrops(true);
    table_widget.setDropIndicatorShown(true);
//    table_widget.setDragDropMode(QAbstractItemView::InternalMove);
    table_widget.setSortingEnabled(true);
    table_widget.sortItems(0, Qt::AscendingOrder);
    table_widget.setSelectionBehavior(QAbstractItemView::SelectRows);

    table_widget.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QStringList s;
    s.push_back("id");
    s.push_back("type");
    s.push_back("src");

    table_widget.setColumnCount(s.size());
    table_widget.setHorizontalHeaderLabels(s);
    table_widget.verticalHeader()->setVisible(false);
    table_widget.horizontalHeader()->setStretchLastSection(true);
    table_widget.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    add_asset_pushbutton.setText("Add Asset");
    add_asset_pushbutton.setMaximumHeight(30);

    remove_asset_pushbutton.setText("Remove Asset");
    remove_asset_pushbutton.setMaximumHeight(30);

//    QWidget * w = new QWidget(this);
    QGridLayout * v = new QGridLayout();
    v->addWidget(&add_asset_pushbutton,0,0);
    v->addWidget(&remove_asset_pushbutton,0,1);
    v->addWidget(&table_widget,1,0,1,2);
    v->setSpacing(0);
    v->setMargin(1);

    setLayout(v);

    connect(&add_asset_pushbutton, SIGNAL(clicked(bool)), this, SLOT(AddAsset()));
    connect(&remove_asset_pushbutton, SIGNAL(clicked(bool)), this, SLOT(RemoveAsset()));
}

void AssetWindow::Update()
{    
    QPointer <Room> r = game->GetEnvironment()->GetCurRoom();
    if (r != cur_room) {
        cur_room = r;
        update = true;
    }

    if (cur_room) {
        QList <QPointer <Asset> > assets = cur_room->GetAllAssets();

        //get number of assets, refresh if it does not match the number of rows in the table                
        if (assets.size() != table_widget.rowCount()) {
            update = true;
        }    

        if (update) {
            update = false;

            table_widget.blockSignals(true);
            table_widget.model()->blockSignals(true);
            table_widget.clearContents();
            table_widget.setSortingEnabled(false);

            table_widget.setRowCount(assets.size());

            int item_count = 0;
            for (QPointer <Asset> & a : assets) {
                if (a) {
                    table_widget.setItem(item_count, 0, new QTableWidgetItem(a->GetProperties()->GetID()));
                    table_widget.setItem(item_count, 1, new QTableWidgetItem(a->GetProperties()->GetTypeAsString()));
                    table_widget.setItem(item_count, 2, new QTableWidgetItem(a->GetProperties()->GetSrc()));
                }
                item_count++;
            }

            table_widget.blockSignals(false);
            table_widget.model()->blockSignals(false);
            table_widget.setSortingEnabled(true);
        }
    }
}

void AssetWindow::keyReleaseEvent(QKeyEvent * e)
{
    switch (e->key()) {
    case Qt::Key_Delete:
    {
        RemoveAsset();
    }
        break;
    }
}

void AssetWindow::AddAsset()
{
    const QString filename = QFileDialog::getOpenFileName(this, "Add Asset", MathUtil::GetWorkspacePath());
    if (!filename.isNull()) {
        AddAssetGivenPath(QUrl::fromLocalFile(filename).toString());
    }
}

void AssetWindow::AddAssetGivenPath(QString url)
{
    //59.0 - don't bother with dialog, just use the filename as assetid for now
    game->DragAndDrop(url.trimmed(), "Drag+Pin", 0);

    //force an update
    cur_room = NULL;
    Update();
}

void AssetWindow::RemoveAsset()
{
    QModelIndexList sel = table_widget.selectionModel()->selectedRows();

    if (cur_room && !sel.isEmpty()) {
        const int index = sel[0].row();
        QList <QPointer <Asset> > assets = cur_room->GetAllAssets();
        if (index >= 0 && index < assets.size()) {
            cur_room->RemoveAsset(assets[index]);
            cur_room = NULL;
            Update();
        }
    }
}
