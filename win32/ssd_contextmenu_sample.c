
#include "../ssd/ssd_contextmenu.h"

static ssd_cm_item   popup_items_L5[] = 
{
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
   SSD_CM_INIT_ITEM( "L-5 - Item", 2),
};

static ssd_cm_item   popup_items_L4[] = 
{
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_POPUP ( "L-4 - Popup", popup_items_L5),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-4 - Item", 2),
};

static ssd_cm_item   popup_items_L3[] = 
{
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_POPUP ( "L-3 - Popup", popup_items_L4),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - ItemL-3 - ItemL-3 - ItemL-3", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-3 - Item", 2),
};

static ssd_cm_item   popup_items_L2a[] = 
{
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_POPUP ( "L-2a - Popup", popup_items_L3),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
   SSD_CM_INIT_ITEM  ( "L-2a - Item", 2),
};

static ssd_cm_item   popup_items_L2b[] = 
{
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
   SSD_CM_INIT_ITEM( "L-2b - Item", 2),
};

static ssd_cm_item   popup_items_L2c[] = 
{
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
   SSD_CM_INIT_ITEM( "L-2c - Item", 2),
};

static ssd_cm_item popup_items_L1[] = 
{
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_items_L2a),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_items_L2b),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_ITEM  ( "L-1 - Item",   1),
   SSD_CM_INIT_POPUP ( "L-1 - Popup", popup_items_L2c)
};

// Context menu items:
static ssd_cm_item main_menu_items[] = 
{
   SSD_CM_INIT_POPUP ( "Popup", popup_items_L1),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
   SSD_CM_INIT_ITEM  ( "Item",   0),
};

ssd_contextmenu  main_menu = SSD_CM_INIT_MENU( main_menu_items);
