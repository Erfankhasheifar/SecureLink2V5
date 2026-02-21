/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * MIT License – see LICENSE file or https://www.FreeRTOS.org for details.
 */

#ifndef LIST_H
#define LIST_H

#ifndef INC_FREERTOS_H
    #error "FreeRTOS.h must be included before list.h"
#endif

/* ---- Macro for list integrity checking --------------------------------- */
#if ( configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES == 0 )
    #define listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
    #define listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE
    #define listFIRST_LIST_INTEGRITY_CHECK_VALUE
    #define listSECOND_LIST_INTEGRITY_CHECK_VALUE
    #define listSET_FIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )
    #define listSET_SECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE( pxItem )
    #define listSET_LIST_INTEGRITY_CHECK_1_VALUE( pxList )
    #define listSET_LIST_INTEGRITY_CHECK_2_VALUE( pxList )
    #define listTEST_LIST_ITEM_INTEGRITY( pxItem )
    #define listTEST_LIST_INTEGRITY( pxList )
#endif

/* ---- List item structure ----------------------------------------------- */
struct xLIST_ITEM
{
    listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
    configLIST_VOLATILE TickType_t  xItemValue;     /**< Sort value */
    struct xLIST_ITEM * configLIST_VOLATILE pxNext;
    struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;
    void *                                  pvOwner; /**< Owning object (TCB, etc.) */
    struct xLIST * configLIST_VOLATILE      pvContainer; /**< List this item is in */
    listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE
};
typedef struct xLIST_ITEM ListItem_t;

#if ( configUSE_MINI_LIST_ITEM == 1 )
    struct xMINI_LIST_ITEM
    {
        listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE
        configLIST_VOLATILE TickType_t  xItemValue;
        struct xLIST_ITEM * configLIST_VOLATILE pxNext;
        struct xLIST_ITEM * configLIST_VOLATILE pxPrevious;
    };
    typedef struct xMINI_LIST_ITEM MiniListItem_t;
#else
    typedef struct xLIST_ITEM MiniListItem_t;
#endif

/* ---- List structure ---------------------------------------------------- */
typedef struct xLIST
{
    listFIRST_LIST_INTEGRITY_CHECK_VALUE
    volatile UBaseType_t uxNumberOfItems;
    ListItem_t * configLIST_VOLATILE pxIndex; /**< Current list iterator */
    MiniListItem_t xListEnd;                   /**< Sentinel node */
    listSECOND_LIST_INTEGRITY_CHECK_VALUE
} List_t;

/* ---- Macros ------------------------------------------------------------ */
#define configLIST_VOLATILE

#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner ) \
    ( ( pxListItem )->pvOwner = ( void * ) ( pxOwner ) )

#define listGET_LIST_ITEM_OWNER( pxListItem ) \
    ( ( pxListItem )->pvOwner )

#define listSET_LIST_ITEM_VALUE( pxListItem, xValue ) \
    ( ( pxListItem )->xItemValue = ( xValue ) )

#define listGET_LIST_ITEM_VALUE( pxListItem ) \
    ( ( pxListItem )->xItemValue )

#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList ) \
    ( ( ( pxList )->xListEnd ).pxNext->xItemValue )

#define listGET_HEAD_ENTRY( pxList ) \
    ( ( ( pxList )->xListEnd ).pxNext )

#define listGET_NEXT( pxListItem ) \
    ( ( pxListItem )->pxNext )

#define listGET_END_MARKER( pxList ) \
    ( ( ListItem_t const * ) ( &( ( pxList )->xListEnd ) ) )

#define listLIST_IS_EMPTY( pxList ) \
    ( ( BaseType_t ) ( ( pxList )->uxNumberOfItems == ( UBaseType_t ) 0 ) )

#define listCURRENT_LIST_LENGTH( pxList ) \
    ( ( pxList )->uxNumberOfItems )

#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )                                     \
    {                                                                                     \
        List_t * const pxConstList = ( pxList );                                          \
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;                     \
        if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) ) \
        {                                                                                 \
            ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;                 \
        }                                                                                 \
        ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;                                   \
    }

#define listGET_OWNER_OF_HEAD_ENTRY( pxList ) \
    ( ( &( ( pxList )->xListEnd ) )->pxNext->pvOwner )

#define listIS_CONTAINED_WITHIN( pxList, pxListItem ) \
    ( ( BaseType_t ) ( ( pxListItem )->pvContainer == ( void * ) ( pxList ) ) )

#define listLIST_ITEM_CONTAINER( pxListItem ) \
    ( ( pxListItem )->pvContainer )

#define listLIST_IS_INITIALISED( pxList ) \
    ( ( pxList )->xListEnd.xItemValue == portMAX_DELAY )

/* ---- Function prototypes ----------------------------------------------- */
void  vListInitialise( List_t * const pxList );
void  vListInitialiseItem( ListItem_t * const pxItem );
void  vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem );
void  vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem );
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove );

#endif /* LIST_H */
