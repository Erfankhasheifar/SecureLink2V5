/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#include <stdlib.h>
#include "FreeRTOS.h"
#include "list.h"

/* ======================================================================== */

void vListInitialise( List_t * const pxList )
{
    /* Set the head and tail of the list to the sentinel node */
    pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );

    /* Make sure the list end value is the highest possible – so it is always
     * at the end of the sorted list. */
    pxList->xListEnd.xItemValue = portMAX_DELAY;

    /* The list end next and previous both point to itself so an empty list
     * is effectively a circular list with only one sentinel item. */
    pxList->xListEnd.pxNext     = ( ListItem_t * ) &( pxList->xListEnd );
    pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );

    pxList->uxNumberOfItems = ( UBaseType_t ) 0U;
}
/* ----------------------------------------------------------------------- */

void vListInitialiseItem( ListItem_t * const pxItem )
{
    /* Make sure the list item is not recorded as being on a list */
    pxItem->pvContainer = NULL;
}
/* ----------------------------------------------------------------------- */

void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t * const pxIndex = pxList->pxIndex;

    /* Insert a new list item into pxList, but rather than sort the list,
     * makes the new list item the last item to be removed by a call to
     * listGET_OWNER_OF_NEXT_ENTRY(). */
    pxNewListItem->pxNext        = pxIndex;
    pxNewListItem->pxPrevious    = pxIndex->pxPrevious;
    pxIndex->pxPrevious->pxNext  = pxNewListItem;
    pxIndex->pxPrevious          = pxNewListItem;

    /* Remember which list the item is in */
    pxNewListItem->pvContainer = ( void * ) pxList;

    ( pxList->uxNumberOfItems )++;
}
/* ----------------------------------------------------------------------- */

void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t * pxIterator;
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    /* If the list already contains a list item with the same item value then
     * the new list item should be placed after it.  This ensures that TCBs
     * which are stored in ready lists (all of which have the same xItemValue
     * value) get an equal share of the CPU.  However, if the xItemValue is the
     * same as the back marker the iteration will not start – the loop never
     * executes and pxIterator remains pointing to the xListEnd. */
    if( xValueOfInsertion == portMAX_DELAY )
    {
        pxIterator = pxList->xListEnd.pxPrevious;
    }
    else
    {
        for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd );
             pxIterator->pxNext->xItemValue <= xValueOfInsertion;
             pxIterator = pxIterator->pxNext )
        {
            /* There is nothing to do here, just iterating to the wanted
             * insertion position. */
        }
    }

    pxNewListItem->pxNext                  = pxIterator->pxNext;
    pxNewListItem->pxNext->pxPrevious      = pxNewListItem;
    pxNewListItem->pxPrevious              = pxIterator;
    pxIterator->pxNext                     = pxNewListItem;

    /* Remember which list the item is in.  This allows fast removal of the
     * item later. */
    pxNewListItem->pvContainer = ( void * ) pxList;

    ( pxList->uxNumberOfItems )++;
}
/* ----------------------------------------------------------------------- */

UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
    /* The list item knows which list it is in.  Obtain the list from the list
     * item. */
    List_t * const pxList = ( List_t * ) pxItemToRemove->pvContainer;

    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
    pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    /* Make sure the index is left pointing to a valid item. */
    if( pxList->pxIndex == pxItemToRemove )
    {
        pxList->pxIndex = pxItemToRemove->pxPrevious;
    }

    pxItemToRemove->pvContainer = NULL;
    ( pxList->uxNumberOfItems )--;

    return pxList->uxNumberOfItems;
}
