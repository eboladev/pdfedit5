#include "settings.h"
#include "util.h"
#include <iostream>
#include <qfile.h>
#include <qdir.h>

using namespace std;

const QString APP_KEY = "/PDFedit/";

/** private initialization function */
void Settings::init() {
 action_index=1;
 initSettings();
 //DEBUG: aktualni adresar, ve finalni verzi nebude
 // settings.insertSearchPath( QSettings::Unix,".");

 /* DEBUG : will get out very soon*/
 set->beginGroup(APP_KEY);
 if (set->readEntry("debugTrashSaved")!="4") {
  set->writeEntry("debugTrashSaved", "4");
  set->writeEntry("keyboard/CtrlA", "dosomethingeverywhere()");
  set->writeEntry("keyboard/context/CtrlA", "dosomethingincontext()");
  set->writeEntry("gui/items/MainMenu",  "list Main menu,file,help");
  set->writeEntry("gui/items/file", "list File,load,save,neww,closew,quit");
  set->writeEntry("gui/items/help", "list Help,about,index");
  set->writeEntry("gui/items/neww",  "item &New Window, newwindow,Ctrl+N");
  set->writeEntry("gui/items/closew","item &Close Window, closewindow");
  set->writeEntry("gui/items/quit",  "item &Quit, quit");
  set->writeEntry("gui/items/load",  "item &Load, loadFile(),, iconload.png");
  set->writeEntry("gui/items/save",  "item &Save, saveFile(),, iconsave.png");
  set->writeEntry("gui/items/about",  "item &About, about()");
  set->writeEntry("gui/items/index",  "item &Help index, showhelp(index)");
  set->writeEntry("gui/items/MainToolbar",  "list Main Toolbar,load,save");
  set->writeEntry("gui/items/OtherToolbar",  "list Other Toolbar,save,load");
  QStringList vi;
  vi+="MainToolbar";
  vi+="OtherToolbar";
  set->writeEntry("gui/toolbars",vi.join(","));/**/
 }
 set->endGroup();
}

/** creates and inits new QSettings Object */
void Settings::initSettings() {
 set=new QSettings(QSettings::Ini);
 set->insertSearchPath(QSettings::Unix,DATA_PATH);
}

/** flushes settings, saving all changes to disk */
void Settings::flushSettings() {
 delete set;  
 initSettings();
}

/** Default constructor */
Settings::Settings() {
 init();
}

/** Default destructor */
Settings::~Settings() {
 delete set;
}

/** Adds action to menu, returning newly allocated menu Id or existing menu id if action is already present
 
 @param action Name of action
 @return Menu ID of the specified action
 */
int Settings::addAction(const QString action) {
 if (action_map.contains(action)) return action_map[action];
 action_index++;
 action_map[action]=action_index;
 action_map_i[action_index]=action;
 return action_index;
}

/** return action string from given menu ID
 
 @param index Menu ID of action
 @return Name of the specified action

 */
QString Settings::getAction(int index) {
 return action_map_i[index];
}

/** returns icon with given name, loading if necessary and caching for later use */
QPixmap *Settings::getIcon(const QString name) {
 cout << "Loading:" << name << endl; 
 if (iconCache.contains(name)) return iconCache[name];
 QFile f(name);
 if (!f.open(IO_ReadOnly)) {
  cout << "File not found:" << name << endl;
  return NULL;//file not found or unreadable or whatever ...
 }
 QByteArray qb=f.readAll();
 f.close();
 QPixmap *pix=new QPixmap();
 pix->loadFromData (qb,0,0);
 iconCache[name]=pix;
 return pix; 
}

/** load one GUI item from config file, exiting application with fatal error if item not found 
 
 @param name Name of the item to read
 @param root Root key to read from (default will be used if none specified)
 @return line from config file  
 */
QString Settings::readItem(const QString name,const QString root) {
 QString line=set->readEntry(APP_KEY+root+name);
 line=line.simplifyWhiteSpace();
 if (line.length()==0) fatalError("Missing item in config:\n"+root+name);
 return line; 
}

/**
 load one menu item and insert it into parent menu. Recursively load subitems if item is a submenu.

 @param name name of item to be loaded from config file
 @param isRoot TRUE if main menubar is being loaded
 @param parent parent menu item (if isRoot == TRUE, this is the root menubar to add items to)
 */ 
void Settings::loadItem(const QString name,QMenuData *parent,bool isRoot) {
 QPopupMenu *item=NULL;
 if (name=="-" || name=="") { //separator
  parent->insertSeparator();
  return;
 }
 QString line=readItem(name);
 if (line.startsWith("list ")) { // List of values - a submenu, first is name of submenu, others are items in it
  if (!isRoot) {
   item=new QPopupMenu();
  }
  line=line.remove(0,5);
  QStringList qs=explode(',',line);  
  QStringList::Iterator it=qs.begin();
  if (it!=qs.end()) { //add itself as popup menu to parent with given name
   if (!isRoot) parent->insertItem(*it,item);
   ++it;
  } else fatalError("Invalid menu item in config:\n"+line);
  for (;it!=qs.end();++it) { //load all subitems
   if (!isRoot) loadItem(*it,item); else loadItem(*it,parent);
  }
 } else if (line.startsWith("item ")) { // A single item
  line=line.remove(0,5);
  //Format: Caption, Action,[,accelerator, [,menu icon]]
  QStringList qs=explode(',',line);
  if (qs.count()<2) fatalError("Invalid menu item in config:\n"+line);
  QString label=qs[0];
  QString action=qs[1];
  int menu_id=addAction(action);
  parent->insertItem(label,menu_id);
  if (qs.count()>=3 && qs[2].length()>0) { //accelerator specified
   parent->setAccel(QKeySequence(qs[2]),menu_id);
  }
  if (qs.count()>=4 && qs[3].length()>0) { //menu icon specified
   QPixmap *pixmap=getIcon(qs[3]);
   if (pixmap) parent->changeItem(menu_id,*pixmap,label); else cout << "Pixmap missing: " << qs[3] << endl;
  }
 } else { //something invalid
  fatalError("Invalid menu item in config:\n"+line);
 } 
}

/** Loads menubar from configuration bar, and return it
 
 If menubar can't be loaded, the application is terminated
 Missing menu icons are allowed (if it can't be loaded, there will be no pixmap), missing items in configuration are not.

 @param parent QWidget that will contain the menubar
 @return loaded and initialized menubar
 */
QMenuBar *Settings::loadMenu(QWidget *parent) {
 //menubar can't be cached and must be separate for each window (otherwise weird things happen)
 QMenuBar *menubar=new QMenuBar(parent);//Make new menubar
 loadItem(QString("MainMenu"),menubar,TRUE);//create root menu
 return menubar;
}

/** Load single toolbar item and add it to toolbar
 @param tb Toolbar for addition of item
 @param item Item name in configuration file
 */
void Settings::loadToolBarItem(QToolBar *tb,QString item) {
 if (item=="-" || item=="") {
  tb->addSeparator();
  return;
 }
 QString line=readItem(item);
 if (line.startsWith("item ")) { //Format: Tooltip, Action,[,accelerator, [,icon]]
  line=line.remove(0,5);  
  QStringList qs=explode(',',line);
  if (qs.count()<2) fatalError("Invalid toolbar item in config:\n"+line);
  QString label=qs[0];
  QString action=qs[1];
  //....
  //....
  //TODO: merge somwhat code from menu item loading in some common function
 } 
 //TODO: this piece of code is work in progress
}

/**
 Load single toolbar from configuration file

 @param name Toolbar name in configuration file
 @param parent Parent window
 @return loaded toolbar
 */
QToolBar *Settings::loadToolbar(const QString name,QMainWindow *parent) {
 QString line=readItem(name);
 if (line.startsWith("list ")) { // List of values - first is name, others are items in it
  line=line.remove(0,5);
  QStringList qs=explode(',',line);
  QToolBar *tb=NULL;
  QStringList::Iterator it=qs.begin();
  if (it!=qs.end()) { //add itself as popup menu to parent with given name
   tb=new QToolBar(*it,parent);
   ++it;
  } else fatalError("Invalid toolbar item in config:\n"+line);
  for (;it!=qs.end();++it) { //load all subitems
   loadToolBarItem(tb,*it);
  }
  return tb;
 } else {
  fatalError("Invalid toolbar item in config:\n"+line);
  return NULL;
 }
}

/** Load all toolbars from configuration files and add them to parent window
 
 @param parent parent window for toolbar
 */
void Settings::loadToolBars(QMainWindow *parent) {
 /* This code is not working now anyway ... 
 QString line=set->readEntry(APP_KEY+"gui/toolbars");
 QStringList tool=explode(',',line);
 for (unsigned int i=0;i<tool.count();i++) {
  loadToolbar(tool[i],parent);
 }
 */
}
