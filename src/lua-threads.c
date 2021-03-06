#include "main.h"

#define MAX_LUA_THREAD_COUNT 10000

typedef struct lua_thread_link_s {
    lua_State *L;
    struct lua_thread_link_s *next;
} lua_thread_link_t;

static int lua_thread_count = 0;
static lua_thread_link_t *lua_thread_head = NULL;
static lua_thread_link_t *lua_thread_tail = NULL;

void reinit_lua_thread_env(lua_State *L)
{
    lua_getglobal(L, "_G");
    lua_getglobal(L, "__INDEXS");
    lua_pushnil(L);

    while(lua_next(L, -2)) {
        if(lua_type(L, -2) == LUA_TSTRING) {
            //printf("* %s (%s) = %s\n", lua_tostring(L, -2), lua_typename(L, lua_type(L, -1)), lua_typename(L, lua_type(L, -3)));
            lua_pushvalue(L, -2);
            lua_pushnil(L);
            lua_rawset(L, -6); //set _G

            lua_pushvalue(L, -2);
            lua_pushnil(L);
            lua_rawset(L, -5); //set __INDEXS
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 2);
}

void release_lua_thread(lua_State *L)
{
    lua_thread_link_t *l = malloc(sizeof(lua_thread_link_t));

    if(!l) {
        LOGF(ERR, "malloc error!");
        return;
    }

    l->L = L;
    l->next = NULL;

    if(lua_thread_tail) {
        lua_thread_tail->next = l;

    } else {
        lua_thread_head = l;
    }

    lua_thread_tail = l;
}

static int l_env_newindex(lua_State *L)
{
    size_t len = 0;
    const char *key = lua_tolstring(L, 2, &len);
    lua_settop(L, 3);
    lua_rawset(L, -3);
    {
        lua_getglobal(L, "__INDEXS");

        if((len < 2 || (key[0] != '_' && key[1] != '_')) && lua_istable(L, -1)) {
            lua_pushstring(L, key);
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);
            lua_pop(L, 1);
        }
    }

    return 0;
}

lua_State *new_lua_thread(lua_State *_L)
{
    lua_State *L = NULL;

    if(lua_thread_head) {
        L = lua_thread_head->L;
        void *l = lua_thread_head;
        lua_thread_head = lua_thread_head->next;

        if(lua_thread_tail == l) {
            lua_thread_head = NULL;
            lua_thread_tail = NULL;
        }

        free(l);
        return L;
    }

    if(lua_thread_count >= MAX_LUA_THREAD_COUNT) {
        LOGF(ERR, "Lua thread pool full!");
        return NULL;
    }

    lua_thread_count++;
    ///lua_getglobal(_L, "cothreads");
    L = lua_newthread(_L);
    ///lua_rawseti(_L, -2, lua_thread_count);
    ///lua_pop(_L, 1);

    int m_refkey = luaL_ref(_L, LUA_REGISTRYINDEX);
    // when you want to kill it.
    //lua_unref(m_state, m_refkey);
    lua_createtable(L, 0, 100);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "_G");

    lua_newtable(L);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_settable(L, -3);

    lua_pushcfunction(L, l_env_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
    lua_replace(L, LUA_GLOBALSINDEX);

    lua_getglobal(L, "__main");
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfenv(L, -2);

    if(lua_resume(L, 0) == LUA_ERRRUN && lua_isstring(L, -1)) {
        LOGF(ERR, "Lua:error %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    lua_createtable(L, 0, 100);
    lua_setglobal(L, "__INDEXS");

    return L;
}

void init_lua_threads(lua_State *_L, int count)
{
    int i = 0;
    void *LS = malloc(sizeof(void *)*count);

    for(i = 0; i < count; i++) {
        void **L = LS + (i * sizeof(void *));
        *L = new_lua_thread(_L);
    }

    for(i = 0; i < count; i++) {
        void **L = LS + (i * sizeof(void *));

        if(*L) {
            release_lua_thread(*L);
        }
    }

    free(LS);
}
