#ifndef __TREEITEMCONTENTSTREAM_H__
#define __TREEITEMCONTENTSTREAM_H__

#include <iproperty.h>
#include <qlistview.h>
#include <ccontentstream.h>
#include "treeitem.h"
class QString;

namespace gui {

using namespace pdfobjects;

class TreeData;

class TreeItemContentStreamObserver;

/** What should be shown as children under this tree item */
typedef enum { All, Text, Font } TreeItemContentStreamMode;

class TreeItemContentStream : public TreeItemAbstract {
public:
 boost::shared_ptr<CContentStream> getObject();
 TreeItemContentStream(TreeData *_data,QListView *parent,boost::shared_ptr<CContentStream> pdfObj,const QString name=QString::null,QListViewItem *after=NULL,const QString &nameId=NULL);
 TreeItemContentStream(TreeData *_data,QListViewItem *parent,boost::shared_ptr<CContentStream> pdfObj,const QString name=QString::null,QListViewItem *after=NULL,const QString &nameId=NULL);
 virtual ~TreeItemContentStream();
 virtual void setOpen(bool open);
 //From TreeItemAbstract interface
 virtual bool validChild(const QString &name,QListViewItem *oldChild);
 virtual ChildType getChildType(const QString &name);
 virtual TreeItemAbstract* createChild(const QString &name,ChildType typ,QListViewItem *after=NULL);
 virtual QStringList getChildNames();
 virtual bool haveChild();
 virtual QSCObject* getQSObject();
 virtual void remove();
 virtual void reloadSelf();
 void setMode(TreeItemContentStreamMode newMode);
 void setMode(const QString &newMode);
private:
 void showMode();
 void initObserver();
 void uninitObserver();
private:
 void init(const QString &name);
 /**  ContentStream object held in this item */
 boost::shared_ptr<CContentStream> obj;
 /** Vector with pdf operators */
 std::vector<boost::shared_ptr<PdfOperator> > op;
 /** Observer registered for this item */
 boost::shared_ptr<TreeItemContentStreamObserver> observer;
 /** Mode - what should be shown? */
 TreeItemContentStreamMode mode;
};

} // namespace gui

#endif
