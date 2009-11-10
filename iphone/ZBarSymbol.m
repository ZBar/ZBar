//------------------------------------------------------------------------
//  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------

#import <zbar/ZBarSymbol.h>

@implementation ZBarSymbol

@dynamic type, typeName, data, quality, count, zbarSymbol;

- (id) initWithSymbol: (zbar_symbol_t*) sym
{
    if(self = [super init]) {
        symbol = sym;
        zbar_symbol_ref(sym, 1);
    }
    return(self);
}

- (void) dealloc
{
    if(symbol) {
        zbar_symbol_ref(symbol, -1);
        symbol = NULL;
    }
    [super dealloc];
}

- (zbar_symbol_type_t) type
{
    return(zbar_symbol_get_type(symbol));
}

- (NSString*) typeName
{
    zbar_symbol_type_t type = zbar_symbol_get_type(symbol);
    return([NSString stringWithUTF8String: zbar_get_symbol_name(type)]);
}

- (NSString*) data
{
    return([NSString stringWithUTF8String: zbar_symbol_get_data(symbol)]);
}

- (int) quality
{
    return(zbar_symbol_get_quality(symbol));
}

- (int) count
{
    return(zbar_symbol_get_count(symbol));
}

- (const zbar_symbol_t*) zbarSymbol
{
    return(symbol);
}

- (ZBarSymbolSet*) components
{
    return([[[ZBarSymbolSet alloc]
                initWithSymbolSet: zbar_symbol_get_components(symbol)]
               autorelease]);
}

@end


@implementation ZBarSymbolSet

@dynamic count, zbarSymbolSet;

- (id) initWithSymbolSet: (const zbar_symbol_set_t*) s
{
    if(self = [super init]) {
        set = s;
        zbar_symbol_set_ref(s, 1);
    }
    return(self);
}

- (void) dealloc
{
    if(set) {
        zbar_symbol_set_ref(set, -1);
        set = NULL;
    }
    [super dealloc];
}

- (int) count
{
    return(zbar_symbol_set_get_size(set));
}

- (const zbar_symbol_set_t*) zbarSymbolSet
{
    return(set);
}

- (NSUInteger) countByEnumeratingWithState: (NSFastEnumerationState*) state
                                   objects: (id*) stackbuf
                                     count: (NSUInteger) len
{
    const zbar_symbol_t *sym = (void*)state->state; // FIXME
    if(sym)
        sym = zbar_symbol_next(sym);
    else if(set)
        sym = zbar_symbol_set_first_symbol(set);

    if(sym)
        *stackbuf = [[[ZBarSymbol alloc]
                         initWithSymbol: sym]
                        autorelease];

    state->state = (unsigned long)sym; // FIXME
    state->itemsPtr = stackbuf;
    state->mutationsPtr = (void*)self;
    return((sym) ? 1 : 0);
}

@end
