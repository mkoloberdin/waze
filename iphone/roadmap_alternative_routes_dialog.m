/* roadmap_alternative_routes_dialog.m - iPhone dialog for alternative routes
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "roadmap_types.h"
#include "roadmap_plugin.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_lang.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_start.h"
#include "roadmap_trip.h"
#include "roadmap_device_events.h"
#include "roadmap_keyboard.h"
#include "roadmap_math.h"
#include "roadmap_messagebox.h"
#include "roadmap_pointer.h"
#include "roadmap_object.h"
#include "address_search/address_search_defs.h"
#include "ssd_progress_msg_dialog.h"
#include "widgets/iphoneLabel.h"
#include "widgets/iphoneCell.h"
#include "widgets/iphoneTableHeader.h"
#include "widgets/iphoneTableFooter.h"
#include "RealtimeAltRoutes.h"
#include "roadmap_alternative_routes.h"
#include "roadmap_alternative_routes_dialog.h"




#define ALT_ROUTES_TITLE "Compare routes"

enum currently_showing {
   SHOWING_NONE,
   SHOWING_ROUTES_LIST,
   SHOWING_ALL_ROUTES,
   SHOWING_ROUTE
};


static CGPoint gPosIcon = {10, 10};
static int gTimeOriginY = 10;
static int gDistanceOriginY = 35;
static int gDescriptionOriginY = 55;
static int gLeftOriginX = 55;
static int gTimeHeight = 25;
static int gDistanceHeight = 20;
static int gDescriptionHeight = 50;
static int gFrequentHeight = 20;





void roadmap_alternative_routes_routes_dialog (void) {
   ssd_progress_msg_dialog_hide ();
   ssd_dialog_hide_all(dec_cancel);
   RoadMapAlternativeRoutesView *view = [[RoadMapAlternativeRoutesView alloc] init];
   [view showListOfRoutes];
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int override_long_click (RoadMapGuiPoint *point) {
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void on_show_route_highlight_dlg_closed () {
   navigate_main_set_outline (0, NULL, 0, -1, FALSE);
   navigate_main_set_outline (1, NULL, 0, -1, FALSE);
   navigate_main_set_outline (2, NULL, 0, -1, FALSE);
   roadmap_math_set_min_zoom(-1);
   
   roadmap_trip_set_focus ("GPS");
   roadmap_pointer_unregister_long_click(override_long_click);
   roadmap_object_enable_short_click();
   ssd_dialog_hide("RouteSelection", dec_close);   
   roadmap_start_screen_refresh(FALSE);
   roadmap_main_set_canvas();
}

int roadmap_alternative_route_select (int index) {
   address_info ai;

   on_show_route_highlight_dlg_closed();
   
   AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   
   if (!pAltRoute)
      return 0;
   
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   
   NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[index];
   navigate_main_set_outline (0, pRouteResults->geometry.points,
                              pRouteResults->geometry.num_points, pRouteResults->alt_id, FALSE);
   roadmap_math_set_min_zoom(-1);
   navigate_main_set_route(pRouteResults->alt_id);
   navigate_route_select(pRouteResults->alt_id);
   ssd_dialog_hide_all (dec_close);
   
   roadmap_log (ROADMAP_INFO,"on_route_selected selecting route alt_id=%d" , pAltRoute->pRouteResults[0].alt_id); 
   
   roadmap_main_show_root(NO);
   
   ai.city = NULL;
   ai.country = NULL;
   ai.house = NULL;
   ai.state= NULL;
   ai.street = NULL;
   Realtime_ReportOnNavigation(&pAltRoute->destPosition, &ai);

   ssd_progress_msg_dialog_show( roadmap_lang_get( "Please wait..." ) );
   
   return 1;
}



//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapAlternativeRoutesView

- (id)init
{	
	self =  [super init];

   routesListView = NULL;
   showAllButton = NULL;
   showListButton = NULL;
   rightButtonView = NULL;
   
   currentlyShowing = SHOWING_NONE;
   
   containerView = [[UIView alloc] initWithFrame:CGRectZero];
   
   self.view = containerView;
   
	
	return self;
}


- (void) resizeViews
{
   UIView *view = [[containerView subviews] objectAtIndex:0];
   view.frame = containerView.bounds;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)viewWillAppear:(BOOL)animated {
   [self resizeViews];
}



- (void) animateToList
{
   //view
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.7f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   
   [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView:containerView cache:YES];
   
   [[[containerView subviews] objectAtIndex:0] removeFromSuperview];
   [containerView addSubview:routesListView];
   
   [UIView commitAnimations];
   
   
   //button
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.7f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   
   [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromLeft forView:rightButtonView cache:YES];
   
   [showListButton removeFromSuperview];
   [rightButtonView addSubview:showAllButton];
   
   [UIView commitAnimations];
}

- (void) animateToMap
{
   //view
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.7f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   
   [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight forView:containerView cache:YES];
   
   [[[containerView subviews] objectAtIndex:0] removeFromSuperview];
   [containerView addSubview:roadmap_main_get_canvas()];
   
   [UIView commitAnimations];
   
   //button
   [UIView beginAnimations:NULL context:NULL];
   [UIView setAnimationDuration:0.7f];
   [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
   
   [UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight forView:rightButtonView cache:YES];
   
   [showAllButton removeFromSuperview];
   [rightButtonView addSubview:showListButton];
   
   [UIView commitAnimations];
}

- (void) onShowList
{
   [self animateToList];
   [self resizeViews];
   self.title = [NSString stringWithUTF8String:roadmap_lang_get(ALT_ROUTES_TITLE)];
   
   
   // Right button
   UINavigationItem *navItem = [self navigationItem];
   navItem.hidesBackButton = NO;
	//[navItem setRightBarButtonItem:showAllButton];
   
   currentlyShowing = SHOWING_ROUTES_LIST;
   on_show_route_highlight_dlg_closed();
}

- (void) onShowAll
{
   // Prepare routes display
   AltRouteTrip *pAltRoute;
   int i;
   int num_routes;
   
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      //todo
      return;
   }
   num_routes = pAltRoute->iNumRoutes;
   
   for (i = 0; i < num_routes; i++) {
      NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[i];
      navigate_main_set_outline (i, pRouteResults->geometry.points,
                                 pRouteResults->geometry.num_points, pRouteResults->alt_id, TRUE);
   }
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   roadmap_trip_set_focus ("Alt-Routes");
   roadmap_pointer_register_long_click (override_long_click, POINTER_HIGHEST);
   roadmap_object_disable_short_click();
   
   // Right button
   UINavigationItem *navItem = [self navigationItem];
   navItem.hidesBackButton = YES;
	//[navItem setRightBarButtonItem:showListButton];
   //rightButton.action = @selector(onShowList);
   
   //[routesListView removeFromSuperview];
   //[containerView addSubview:roadmap_main_get_canvas()];
   [self animateToMap];
   roadmap_start_screen_refresh(TRUE);
   currentlyShowing = SHOWING_ALL_ROUTES;
   [self resizeViews];
   add_routes_selection(NULL);
   highligh_selection(-1);
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);
   roadmap_screen_redraw();
}

- (void) onShowRoute: (int) index
{
   // Prepare routes display
   char title[20];
   AltRouteTrip *pAltRoute;
   
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   if (!pAltRoute) {
      //todo
      return;
   }
   
   //sprintf (title, roadmap_lang_get ("Route %d"), index + 1);
   //self.title = [NSString stringWithUTF8String:title];
   
   NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[index];
   navigate_main_set_outline (index, pRouteResults->geometry.points,
                              pRouteResults->geometry.num_points, pRouteResults->alt_id, TRUE);
   
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   roadmap_trip_set_focus ("Alt-Routes");
   roadmap_pointer_register_long_click (override_long_click, POINTER_HIGHEST);
   roadmap_object_disable_short_click();
   
   // Right button
   UINavigationItem *navItem = [self navigationItem];
   navItem.hidesBackButton = YES;

   [self animateToMap];
   roadmap_start_screen_refresh(TRUE);
   currentlyShowing = SHOWING_ROUTE;
   [self resizeViews];
   add_routes_selection(NULL);
   highligh_selection(index);
   roadmap_math_set_min_zoom(ALT_ROUTESS_MIN_ZOOM_IN);
   roadmap_screen_redraw();
}


- (void) createListView
{
   if (routesListView != NULL)
      return;
   
   routesListView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStyleGrouped];
   routesListView.dataSource = self;
   routesListView.delegate = self;
   [routesListView setBackgroundColor:[UIColor clearColor]];
   routesListView.rowHeight = 110;
}

- (void) createShowListButton
{
   
   UIImage *image;
   CGRect rect;
   
   if (showListButton == NULL) {
      image = roadmap_iphoneimage_load("route_list_e");
      if (image) {
         rect.origin = CGPointMake (0,0);
         rect.size = image.size;
         showListButton = [[UIButton alloc] initWithFrame:rect];
         [showListButton setImage:image forState: UIControlStateNormal];
         [showListButton addTarget:self action:@selector(onShowList) forControlEvents:UIControlEventTouchUpInside];
         [image release];
      }
   }
   /*
    if (showListButton == NULL)
    showListButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("List")]
    style:UIBarButtonItemStyleDone target:self action:@selector(onShowList)];
    */
}

- (void) createShowAllButton
{
   UIImage *image;
   CGRect rect;
   
   if (showAllButton == NULL) {
      image = roadmap_iphoneimage_load("route_map_e");
      if (image) {
         rect.origin = CGPointMake (0,0);
         rect.size = image.size;
         showAllButton = [[UIButton alloc] initWithFrame:rect];
         [showAllButton setImage:image forState: UIControlStateNormal];
         [showAllButton addTarget:self action:@selector(onShowAll) forControlEvents:UIControlEventTouchUpInside];
         [image release];
      }
   }
      //showAllButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("All")]
        //                                                      style:UIBarButtonItemStyleDone target:self action:@selector(onShowAll)];
}

- (void) showListOfRoutes
{
   self.title = [NSString stringWithUTF8String:roadmap_lang_get(ALT_ROUTES_TITLE)];
   [self createListView];
   [self createShowAllButton];
   [self createShowListButton];
   
   [containerView addSubview: routesListView];
   
   
   // Right button
   UINavigationItem *navItem = [self navigationItem];
   rightButtonView = [[UIView alloc] initWithFrame: showAllButton.bounds];
   [rightButtonView addSubview:showAllButton];
   rightButton  = [[UIBarButtonItem alloc] initWithCustomView:rightButtonView];
	[navItem setRightBarButtonItem:rightButton];
   
   currentlyShowing = SHOWING_ROUTES_LIST;
   
   roadmap_main_push_view(self);
}



- (UIView *) createCellView: (int)index frame: (CGRect) viewRect
{
   char icon[20];
   char msg[300];
   UIImage *image;
   UIImageView *imageView;
   iphoneLabel *label;
   UIView *view = [[UIView alloc] initWithFrame:viewRect];
   NavigateRouteResult *nav_result = RealtimeAltRoutes_Get_Route_Result (index);
   CGRect rect;
   int labels_width = viewRect.size.width - gLeftOriginX - 15;
   
   [view setAutoresizesSubviews:YES];
   [view setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
   
   //icon
   sprintf (icon, "%d_route", index + 1);
   image = roadmap_iphoneimage_load(icon);
   if (image) {
      imageView = [[UIImageView alloc] initWithImage:image];
      [image release];
      rect = imageView.bounds;
      rect.origin = gPosIcon;
      imageView.frame = rect;
      
      [view addSubview:imageView];
      [imageView release];
   }
   
   //time
   msg[0] = 0;
   snprintf (msg , sizeof (msg), "%.1f %s",
             nav_result->total_time / 60.0, roadmap_lang_get ("min."));
   rect.origin.y = gTimeOriginY;
   rect.origin.x = gLeftOriginX;
   rect.size.height = gTimeHeight;
   rect.size.width = labels_width;
   label = [[iphoneLabel alloc] initWithFrame:rect];
   label.text = [NSString stringWithUTF8String:msg];
   [label setAutoresizesSubviews:YES];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
   [label setFont:[UIFont boldSystemFontOfSize:25]];
   [label setAdjustsFontSizeToFitWidth:YES];
   //label.backgroundColor = [UIColor redColor];
   label.backgroundColor = [UIColor clearColor];
   [view addSubview: label];
   [label release];
   
   //distance
   msg[0] = 0;
   snprintf (msg + strlen (msg), sizeof (msg) - strlen (msg), "%.1f %s",
             nav_result->total_length / 1000.0,
             roadmap_lang_get (roadmap_math_trip_unit ()));
   rect.origin.y = gDistanceOriginY;
   rect.origin.x = gLeftOriginX;
   rect.size.height = gDistanceHeight;
   rect.size.width = labels_width;
   label = [[iphoneLabel alloc] initWithFrame:rect];
   label.text = [NSString stringWithUTF8String:msg];
   [label setAutoresizesSubviews:YES];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
   [label setFont:[UIFont systemFontOfSize:18]];
   [label setAdjustsFontSizeToFitWidth:YES];
   //label.backgroundColor = [UIColor blueColor];
   label.backgroundColor = [UIColor clearColor];
   [view addSubview: label];
   [label release];
   
   //description
   msg[0] = 0;
   snprintf (msg, sizeof (msg), "%s: %s",
             roadmap_lang_get("Via"),
             roadmap_lang_get (nav_result->description));
   rect.origin.y = gDescriptionOriginY;
   rect.origin.x = gLeftOriginX;
   rect.size.height = gDescriptionHeight;
   rect.size.width = labels_width;
   label = [[iphoneLabel alloc] initWithFrame:rect];
   label.text = [NSString stringWithUTF8String:msg];
   [label setAutoresizesSubviews:YES];
   [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
   [label setFont:[UIFont systemFontOfSize:16]];
   label.textColor = [UIColor darkGrayColor];
   [label setAdjustsFontSizeToFitWidth:YES];
   [label setNumberOfLines:0];
   [label setLineBreakMode:UILineBreakModeWordWrap];
   //label.backgroundColor = [UIColor yellowColor];
   label.backgroundColor = [UIColor clearColor];
   [view addSubview: label];
   [label release];
   
   //your frequent
   if (nav_result->origin){
      rect.origin.y = gTimeOriginY;
      rect.origin.x = gLeftOriginX;
      rect.size.height = gFrequentHeight;
      rect.size.width = labels_width;
      label = [[iphoneLabel alloc] initWithFrame:rect];
      label.text = [NSString stringWithUTF8String:roadmap_lang_get("(your frequent)")];
      if (label.textAlignment == UITextAlignmentRight)
         label.textAlignment = UITextAlignmentLeft;
      else
         label.textAlignment = UITextAlignmentRight;
      [label setAutoresizesSubviews:YES];
      [label setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
      [label setFont:[UIFont systemFontOfSize:14]];
      [label setAdjustsFontSizeToFitWidth:YES];
      //label.backgroundColor = [UIColor blueColor];
      label.backgroundColor = [UIColor clearColor];
      [view addSubview: label];
      [label release];
   }
   
   return view;
}


- (void)dealloc
{
 
      
   
   if (routesListView)
      [routesListView release];
   if (showAllButton)
      [showAllButton release];
   if (showListButton)
      [showListButton release];
   if (rightButtonView)
      [rightButtonView release];
   
   [containerView release];
   
	[super dealloc];
   
   
   // if (currentlyShowing != SHOWING_ROUTES_LIST) {
   
   //}
}




//////////////////////
//TableView delegate
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	
   //return RealtimeAltRoutes_Get_Num_Routes ();
   return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
   
   return RealtimeAltRoutes_Get_Num_Routes ();
}



- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	iphoneCell *cell = NULL;
	
   //cell = (iphoneCell *)[tableView dequeueReusableCellWithIdentifier:@"altRoutesCell"];
   //if (cell == nil) {
      cell = [[[iphoneCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"altRoutesCell"] autorelease];
   //}
   //if ([cell.contentView.subviews count] > 0)
   //   [[cell.contentView.subviews objectAtIndex:0] release];//TODO: reuse
   //[cell.contentView addSubview:[self createCellView:indexPath.section frame:cell.bounds]];
   [cell.contentView addSubview:[self createCellView:indexPath.row frame:cell.bounds]];
   cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
	
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	AltRouteTrip *pAltRoute;
   pAltRoute = (AltRouteTrip *) RealtimeAltRoutes_Get_Record (0);
   
   if (!pAltRoute)
      return;
   
   roadmap_trip_set_point ("Destination", &pAltRoute->destPosition);
   roadmap_trip_set_point ("Departure", &pAltRoute->srcPosition);
   
   //NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[indexPath.section];
   NavigateRouteResult *pRouteResults = &pAltRoute->pRouteResults[indexPath.row];
   navigate_main_set_outline (0, pRouteResults->geometry.points,
                              pRouteResults->geometry.num_points, pRouteResults->alt_id, FALSE);
   roadmap_math_set_min_zoom(-1);
   ssd_dialog_hide_all (dec_close);
   
   roadmap_log (ROADMAP_INFO,"on_route_selected selecting route alt_id=%d" , pAltRoute->pRouteResults[0].alt_id); 
   navigate_main_set_route(pRouteResults->alt_id);
   navigate_route_select(pRouteResults->alt_id);
   roadmap_main_show_root(NO);
   
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Please wait..." ) );
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
   //[self onShowRoute:indexPath.section];
   [self onShowRoute:indexPath.row];
}



- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
   iphoneTableHeader *header = NULL;
   
   /*if (section == 0)*/ {
      header = [[[iphoneTableHeader alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)] autorelease];
      [header setText:"Recommended routes:"];
   }
   
   
   return header;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
   
   return 30;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
   iphoneTableFooter *footer = NULL;
   
   footer = [[[iphoneTableFooter alloc] initWithFrame:CGRectMake(IPHONE_TABLE_INIT_RECT)] autorelease];
   
   if (RealtimeAltRoutes_Get_Num_Routes () == 1)
      [footer setText:roadmap_lang_get("No valid alternatives were found for this destination")];
   else
      [footer setText:roadmap_lang_get("Recommended route may take a bit more time but has less turns and junctions")];
   
   
   return footer;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
   //if (section == RealtimeAltRoutes_Get_Num_Routes() - 1)
      return 90;
   //else
   //   return 0;
}



@end

