/** @file
 RefPropertyDialog - interactive reference target picker
 @author Martin Petricek
*/

#include "refpropertydialog.h"
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qvalidator.h>
#include <qstring.h>
#include "pdfutil.h"

//#include <utils/debug.h>
//#include <stdlib.h>
//#include <cobject.h>
//#include "util.h"

namespace gui {

using namespace std;
using namespace util;

/**
 Default constructor of reference picker dialog
 @param _pdf Pdf in which is the reference
 @param src original value of indirect refrence
 @param parent Parent widget that called dialog
 */
RefPropertyDialog::RefPropertyDialog(CPdf* _pdf,IndiRef src, QWidget *parent/*=0*/) : QDialog (parent,"refproperty_dialog",true) {
 pdf=_pdf;
 value=src;
 setCaption(tr("Reference target"));
 QGridLayout *l=new QGridLayout(this,4,2,8);
 
 //The prompt
 QLabel *lbDesc=new QLabel(tr("Choose target for indirect reference"), this);
 l->addMultiCellWidget(lbDesc,0,0,0,1);

 //Edit widgets
 QLabel *lbNum=new QLabel(tr("Object number"), this);
 QLabel *lbGen=new QLabel(tr("Generation number"), this);
 QIntValidator *v=new QIntValidator(this);
 v->setBottom(0);
 QLineEdit *edNum=new QLineEdit(QString::number(src.num), this);
 edNum->setValidator(v);
 QLineEdit *edGen=new QLineEdit(QString::number(src.gen), this);
 edGen->setValidator(v);
 l->addWidget(lbNum,1,0);
 l->addWidget(edNum,1,1);
 l->addWidget(lbGen,2,0);
 l->addWidget(edGen,2,1);

 //Error/informational message
 infoText=new QLabel("", this);
 l->addMultiCellWidget(infoText,3,3,0,1);

 //Lower frame with Ok & Cancel button
 QFrame *okFrame=new QFrame(this);
 QGridLayout *lFrame=new QGridLayout(okFrame,1,2,5);
 QPushButton *ok=new QPushButton(QObject::tr("&Ok"), okFrame);
 lFrame->addWidget(ok,0,0);
 QPushButton *cancel=new QPushButton(QObject::tr("&Cancel"), okFrame);
 lFrame->addWidget(cancel,0,1);
 l->addMultiCellWidget(okFrame,4,4,0,1);

 //Connect signals
 QObject::connect(ok, SIGNAL(clicked()), this, SLOT(maybeAccept()));
 QObject::connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
 QObject::connect(edNum, SIGNAL(textChanged(const QString &)), this, SLOT(setNum(const QString &)));
 QObject::connect(edGen, SIGNAL(textChanged(const QString &)), this, SLOT(setGen(const QString &)));

 check();
}

/**
 If input values are ok, call accept(),
 otherwise display an error message
*/
void RefPropertyDialog::maybeAccept() {
 if (!check(false)) return;
 accept();
}

/** 
 Show informational message for this window
 @param message message to show
*/
void RefPropertyDialog::message(const QString &message) {
 infoText->setPaletteForegroundColor(QColor(0,0,0));//Set black color
 infoText->setText(message);
}

/** 
 Show error message for this window
 @param message message to show
*/
void RefPropertyDialog::error(const QString &message) {
 infoText->setPaletteForegroundColor(QColor(255,0,0));//Set red color
 infoText->setText(message);
}

/**
 Check stored value for validity, return true if valid,
 otherwise display an error message and return false.
 @param sayIfOk If set to false, no message will be output if the property is valid
*/
bool RefPropertyDialog::check(bool sayIfOk/*=true*/) {
 assert(pdf);
 if (!isRefValid(pdf,value)) { 
  //Not valid
  error(tr("Reference target is not valid"));
  return false;
 }
 //Is valid
 if (!sayIfOk) return true;
 boost::shared_ptr<IProperty> rp=pdf->getIndirectProperty(value); 
 QString description=propertyPreview(rp);
 if (description.length()) {
  description=" : "+description;
 }
 message(getTypeName(rp)+" "+description);
 return true;
}

/** 
 Return result of this dialog as string
 @return reference as string
*/
QString RefPropertyDialog::getResult() {
 return QString::number(value.num)+","+QString::number(value.gen);
}

/**
 Set reference number - called from corresponing edit control
 @param text value to set
*/
void RefPropertyDialog::setNum(const QString &text) {
 value.num=text.toUInt();
 check();
}

/**
 Set generation number - called from corresponing edit control
 @param text value to set
*/
void RefPropertyDialog::setGen(const QString &text) {
 value.gen=text.toUInt();
 check();
}

/** default destructor */
RefPropertyDialog::~RefPropertyDialog() {
}

} // namespace gui
