#import "roadmap_address_book.h"
#include "roadmap_main.h"
#include "roadmap_iphonemain.h"

#import <AddressBook/AddressBook.h>
static RoadmapAddressBook *addressbook; 

void roadmap_address_book(SsdKeyboardCallback callback, void *context)
{
	addressbook = [RoadmapAddressBook alloc];
    [addressbook pickname:callback 
				andContext:context];
}

@implementation RoadmapAddressBook
@synthesize gContext;
@synthesize gCallback;

- (void)pickname:(SsdKeyboardCallback) callback
	  andContext: (void *)context
{
	gCallback = callback;
	gContext = context;
    ABPeoplePickerNavigationController *ppnc = [[ABPeoplePickerNavigationController alloc] init];
    ppnc.peoplePickerDelegate = self;
    ppnc.displayedProperties = [NSArray arrayWithObject:[NSNumber numberWithInt:kABPersonAddressProperty]];
	//[ppnc setDisplayedProperties:[NSArray arrayWithObjects: [NSNumber numberWithInt: kABPersonPhoneProperty], [NSNumber numberWithInt:kABPersonAddressProperty], nil]];
	
    //[self presentModalViewController:ppnc animated:YES];
	roadmap_main_present_modal (ppnc);
    [ppnc release];
}

- (void)dealloc {
    [super dealloc];
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker
{
	roadmap_main_dismiss_modal();
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person
{
	return YES;
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker shouldContinueAfterSelectingPerson:(ABRecordRef)person property:(ABPropertyID)property identifier:(ABMultiValueIdentifier)identifier
{
	roadmap_main_dismiss_modal();
    
	if (property == kABPersonAddressProperty){
		if (person) {
			ABMultiValueRef addresses = ABRecordCopyValue(person, kABPersonAddressProperty);
			if (addresses) {
				CFIndex index = ABMultiValueGetIndexForIdentifier(addresses, identifier);
				CFDictionaryRef address = ABMultiValueCopyValueAtIndex(addresses, index);
			
				NSString *street = [(NSString *)CFDictionaryGetValue(address, kABPersonAddressStreetKey) copy];
				NSString *city = [(NSString *)CFDictionaryGetValue(address, kABPersonAddressCityKey) copy];
				NSString *state = [(NSString *)CFDictionaryGetValue(address, kABPersonAddressStateKey) copy];
			
				if (street == nil)
					street = @"";
				
				if (city == nil)
					city = @"";
				
				if (state == nil)
					state = @"";
				
				NSString *str = [[NSString alloc] initWithFormat:@"%@ %@ %@", street, city, state];

				const char *myText = [str  UTF8String];
				gCallback (1, myText , gContext);
				CFRelease(address);
				[street autorelease];
				[city autorelease];
				[state autorelease];
				CFRelease(addresses);
        }
    }
	}
	else if (property == kABPersonPhoneProperty) {
		ABMultiValueRef multi = ABRecordCopyValue(person, kABPersonPhoneProperty);
		if (multi){
			CFIndex index = ABMultiValueGetIndexForIdentifier(multi, identifier);
		
			NSString  *phoneNumber= (NSString*)ABMultiValueCopyValueAtIndex(multi, index);
			phoneNumber = [[[[phoneNumber stringByReplacingOccurrencesOfString:@"(" withString:@""]
							 stringByReplacingOccurrencesOfString:@")" withString:@""]
							 stringByReplacingOccurrencesOfString:@"-" withString:@""]
							 stringByReplacingOccurrencesOfString:@" " withString:@""];
		
			NSString *url = [NSString stringWithFormat:@"tel:%@", phoneNumber];
			roadmap_main_open_url([url UTF8String]);
		}
	}
    return NO;
}

@end

