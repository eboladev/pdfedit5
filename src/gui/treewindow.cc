/** @file
 TreeWindow - class providing treeview of PDF objects
 @author Martin Petricek
*/
#include <utils/debug.h>
#include "treewindow.h"
#include "settings.h"
#include "util.h"
#include <iostream>
#include <qlayout.h>
#ifdef DRAGDROP
#include "draglistview.h"
#endif
#include "treedata.h"
#include "pdfutil.h"
#include "treeitempdf.h"
#include "treeitemcontentstream.h"
#include "treeitemoperatorcontainer.h"
#include "treeitem.h"
#include "base.h"
#include <ccontentstream.h>
#include <cpdf.h>

namespace gui {

using namespace std;
using namespace util;

/*
TODO:
 [ ]value-based reloading - check if value of children in reality match stored value, sometimes
    (especially if changing revision, but also on many array-related changes) keys do not change
    but values does
 [ ]array: keys are worthless ... compare only by values
*/
/**
 constructor of TreeWindow, creates window and fills it with elements, parameters are ignored
 @param base Scripting base
 @param parent Parent widget
 @param name Name of this widget (not used, just passed to QWidget)
 @param multi MultiTreeWindow holding this tree
*/
TreeWindow::TreeWindow(MultiTreeWindow *multi,Base *base,QWidget *parent/*=0*/,const char *name/*=0*/):QWidget(parent,name) {
 QBoxLayout *l=new QVBoxLayout(this);
#ifdef DRAGDROP
 tree=new DragListView(this,"tree_view");//DragListView for drag and drop
#else
 tree=new QListView(this,"tree_view");//DragListView for drag and drop
#endif
 tree->setSorting(-1);
 QObject::connect(tree,SIGNAL(selectionChanged(QListViewItem *)),this,SLOT(treeSelectionChanged(QListViewItem *)));
#ifdef DRAGDROP
 QObject::connect(tree,SIGNAL(dragDrop(TreeItemAbstract*,TreeItemAbstract*)),base,SLOT(_dragDrop(TreeItemAbstract*,TreeItemAbstract*)));
 QObject::connect(tree,SIGNAL(dragDropOther(TreeItemAbstract*,TreeItemAbstract*)),base,SLOT(_dragDropOther(TreeItemAbstract*,TreeItemAbstract*)));
#endif
 l->addWidget(tree);
 tree->addColumn(tr("Object"));
 tree->addColumn(tr("Type"));
 tree->addColumn(tr("Data"));
 tree->setSelectionMode(QListView::Single/*Extended*/);
 tree->setColumnWidthMode(0,QListView::Maximum);
 tree->show();
 data=new TreeData(this,tree,base,multi);
 QObject::connect(tree,SIGNAL(mouseButtonClicked(int,QListViewItem*,const QPoint &,int)),this,SLOT(mouseClicked(int,QListViewItem*,const QPoint &,int)));
 QObject::connect(tree,SIGNAL(doubleClicked(QListViewItem*,const QPoint &,int)),this,SLOT(mouseDoubleClicked(QListViewItem*,const QPoint &,int)));
 QObject::connect(tree,SIGNAL(onItem(QListViewItem*)),this,SLOT(moveOnItem(QListViewItem*)));
 QObject::connect(tree,SIGNAL(onViewport()),this,SLOT(moveOffItem()));
}

/**
 Slot called when mouse moves to some item in tree
 @param item Item over which mouse moved
*/
void TreeWindow::moveOnItem(QListViewItem *item) {
 TreeItemAbstract* abst=dynamic_cast<TreeItemAbstract*>(item);
 if (!abst) {
  emit itemInfo("");
  return;
 }
 emit itemInfo(abst->path());
}

/**
 Slot called when mouse moves off any items in tree
*/
void TreeWindow::moveOffItem() {
 emit itemInfo("");
}

/** Reload part of tree that have given item as root (including that item)
 Reloading will stop at unopened reference targets
 @param item root of subtree to reload
Ow */
void TreeWindow::reloadFrom(TreeItemAbstract *item) {
 assert(item);
 item->reload();
}

/** reinitialize/reload entire tree after some major change */
void TreeWindow::reload() {
 TreeItemAbstract *rootItem=root();
 if (!rootItem) return;//Tree is empty, nothing to reload
 rootItem->reload();
}

/**
 Delete notification from tree items
 @param notifyItem Item just being deleted
*/
void TreeWindow::deleteNotify(TreeItemAbstract *notifyItem) {
 //TODO: different in mutiselect
 if (tree->isSelected(notifyItem)) {
  //Unselect item that is just going to be deleted
  tree->setSelected(notifyItem,false);
 }
}

/**
 Return root item of the tree. (or NULL if tree is empty)
 @return root item
 */
TreeItemAbstract* TreeWindow::root() {
 TreeItemAbstract *rootItem=dynamic_cast<TreeItemAbstract *>(tree->firstChild());
 return rootItem;
}

/**
 Slot called when someone click with mouse button anywhere in the tree
 @param button Which button(s) are clicked (1=left, 2=right, 4=middle)
 @param item Which item is clicked upon (NULL if clicked outside item)
 @param coord Coordinates of mouseclick
 @param column Clicked in which item's column? (if clicked on item)
*/
void TreeWindow::mouseClicked(int button,QListViewItem* item,__attribute__((unused)) const QPoint &coord,__attribute__((unused)) int column) {
 emit treeClicked(button,item);
}

/**
 Slot called when someone doubleclick with left mouse button anywhere in the tree
 @param item Which item is clicked upon (NULL if clicked outside item)
 @param coord Coordinates of mouseclick
 @param column Clicked in which item's column? (if clicked on item)
*/
void TreeWindow::mouseDoubleClicked(QListViewItem* item,__attribute__((unused)) const QPoint &coord,__attribute__((unused)) int column) {
 emit treeClicked(8,item);
}

/** Re-read tree settings from global settings */
void TreeWindow::updateTreeSettings() {
 data->update();
 if (data->isDirty()) {
//  guiPrintDbg(debug::DBG_DBG,"update tree settings: is dirty");
  data->resetDirty();
  update();//Update treeview itself
 }
}


/**
 Paint event handler -> if settings have been changed, reload tree
 @param e Paint event data (passed to widget)
*/
void TreeWindow::paintEvent(QPaintEvent *e) {
 if (data->needReload()) {
  guiPrintDbg(debug::DBG_DBG,"update tree settings: need reload");
  reload(); //update object if necessary
  data->resetReload();
 }
 //Pass along
 QWidget::paintEvent(e);
}

/**
 Called when any settings are updated (in script, option editor, etc ...) 
 @param key Key of setting which have changed
*/
void TreeWindow::settingUpdate(QString key) {
 //TODO: only once per bunch of tree/show... signals ... setting blocks
 guiPrintDbg(debug::DBG_DBG,"Settings observer: " << key);
 if (key.startsWith("tree/show")) { //Updated settings of what to show and what not
  updateTreeSettings();
 }
}

/**
 Called upon changing selection in the tree window
 @param item The item that was selected
 */
void TreeWindow::treeSelectionChanged(__attribute__((unused)) QListViewItem *item) {
 emit itemSelected();
}

/** 
 Return QSCObject from currently selected item
 Caller is responsible for freeing object
 @return QSCObject from current item
*/
QSCObject* TreeWindow::getSelected() {
 //Ask the tree for selected item
 TreeItemAbstract *selected=dynamic_cast<TreeItemAbstract*>(tree->selectedItem());
 if (!selected) return NULL; //nothing selected
 return selected->getQSObject();
}

/** 
 Return pointer to currently selected tree item
 @return current item
*/
TreeItemAbstract* TreeWindow::getSelectedItem() {
 //Ask the tree for selected item
 TreeItemAbstract *selected=dynamic_cast<TreeItemAbstract*>(tree->selectedItem());
 if (!selected) return NULL; //nothing selected
 return selected;
}

/** Clears all items from TreeWindow */
void TreeWindow::clear() {
 QListViewItem *li;
 while ((li=tree->firstChild())) {
  delete li;
 }
 data->clear();
}

/** Init contents of treeview from given PDF document
 @param pdfDoc Document used to initialize treeview
 @param fileName Name of PDF document (will be shown in treeview as name of root element)
 */
void TreeWindow::init(pdfobjects::CPdf *pdfDoc,const QString &fileName) {
 assert(pdfDoc);
 clear();
 rootName=fileName;
 setUpdatesEnabled( FALSE );
 TreeItemAbstract *rootItem=new TreeItemPdf(data,pdfDoc,tree,fileName); 
 rootItem->setOpen(TRUE);
 setUpdatesEnabled( TRUE );
}

/**
 Init contents of treeview from given IProperty (dictionary, etc ...)
 @param doc IProperty used to initialize treeview
 @param pName Name of the property passed
*/
void TreeWindow::init(boost::shared_ptr<pdfobjects::IProperty> doc,const QString &pName/*=QString::null*/) {
 clear();
 if (doc.get()) {
  setUpdatesEnabled( FALSE );
  TreeItemAbstract *rootItem=TreeItem::create(data,tree,doc,pName); 
  rootItem->setOpen(TRUE);
  setUpdatesEnabled( TRUE );
 }
}

/**
 Init contents of treeview from given Content Stream 
 @param cs Content Stream used to initialize treeview
 @param pName Name of the content stream passed
*/
void TreeWindow::init(boost::shared_ptr<pdfobjects::CContentStream> cs,const QString &pName/*=QString::null*/) {
 clear();
 if (cs.get()) {
  setUpdatesEnabled( FALSE );
  TreeItemAbstract *rootItem=new TreeItemContentStream(data,tree,cs,pName); 
  rootItem->setOpen(TRUE);
  setUpdatesEnabled( TRUE );
 }
}

/**
 Init contents of treeview from given vector with operators
 @param vec Vector used to initialize treeview
 @param pName Name of the root item
*/
void TreeWindow::init(const OperatorVector &vec,const QString &pName/*=QString::null*/) {
 clear();
 setUpdatesEnabled( FALSE );
 TreeItemAbstract *rootItem=new TreeItemOperatorContainer(data,tree,vec,pName); 
 rootItem->setOpen(TRUE);
 setUpdatesEnabled( TRUE );
}

/** Resets the tree to be empty and show nothing */
void TreeWindow::uninit() {
 clear();
}

/** default destructor */
TreeWindow::~TreeWindow() {
 //Delete all items
 tree->clear();
 //Tree must be deleted first, since some tree items might be using the data during destructor
 delete tree;
 delete data;
}

} // namespace gui
