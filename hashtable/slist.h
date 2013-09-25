#pragma once


SLIST_HEAD(slisthead, entry) g_list = {NULL}, g_listdeleted = {NULL};

void slist_add(entry_t* ent)
{
    SLIST_INSERT_HEAD(&g_list, ent, entries);
}

u16 slist_length()
{
    if(SLIST_EMPTY(&g_list))
        return 0;
    entry_t  *np = NULL, *tp=NULL;
    u16 len = 0;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        ++len;
    }
    return len;
}

entry_t* slist_get_at_index(u16 index)
{
    u16 len = 0;
    entry_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if(len == index){
            return np;
        }
        ++len;
    }
    LOGERR("out of bound: index=%u, len=%u", index, len);
    return NULL;
}

entry_t* slist_find(itnexus_t nexus)
{
    entry_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( itnexus_equal(np->pr.nexus, nexus))
            return np;
    }
    return NULL;
}

entry_t* slist_delete(itnexus_t nexus){
    entry_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( itnexus_equal(np->pr.nexus, nexus))
        {
            SLIST_REMOVE(&g_list, np, entry, entries);
            return np;
        }
    }
    return NULL;
}


