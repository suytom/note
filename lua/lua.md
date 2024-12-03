# <center>Lua源码阅读记录(5.4.7)</center>

## A、数据结构
### 1、TString  
+ TString  
  ![struct_tstring](../lua/struct_tstring.png)  
+ luaS_newlstr  
  ![create_string](../lua/create_string.png)  
  *PS:过长的字符串（长度大于40）每次都会创建一个新对象，大概是是为了性能考虑，如果全放在global_State中的strt中(strt的数据类型为stringtable)，长度过长时性能较差（计算hash值时会遍历整个字符串）。stringtable缩小扩容都是2倍。*
+ internshrstr  
  ![create_short_string](../lua/create_short_string.png)
+ stringtable  
  ![string_table](../lua/string_table.png)

### 2、Table
+ Table  
  ![struct_table](../lua/struct_table.png)
+ table相关操作指令：  
  + OP_NEWTABLE：  
  ![OP_NEWTABLE](../lua/OP_NEWTABLE.png)  
  + OP_SETLIST：  
  ![OP_SETLIST](../lua/OP_SETLIST.png)
  + OP_SETI：  
  ![OP_SETI](../lua/OP_SETI.png)  
  + OP_SETFIELD：  
  ![OP_SETFIELD](../lua/OP_SETFIELD.png)  
  + OP_SETTABLE：  
  ![OP_SETTABLE](../lua/OP_SETTABLE.png)
  + ```local tbl = {1,["test1"] = 4,2,["test2"] = 5,3,["test3"] = 6};```该代码的指令调用：首先调用OP_NEWTABLE，并且b(hash size)、c(array size)不为0，再调用OP_SETFIELD，一个个设置hash部分数据以及OP_SETLIST一次性设置array部分数据。  
  ```local tbl = {}; tbl[1] = 1; tbl["test1"] = 4; tbl[2] = 2;``` 该代码的指令调用：首先调用OP_NEWTABLE，并且b(hash size)、c(array size)为0，再按顺序调用OP_SETI或者OP_SETFIELD，赋值过程中会调整table的数组和hash大小。  
  <font color= "#FF0000">OP_SETTABLE在5.4.7貌似没用了。被拆分为OP_SETI和OP_SETFIELD。</font>  
  + luaV_fastseti（OP_SETI查找位置）：  
    ```c  
    #define luaV_fastseti(t,k,val,hres) \
    if (!ttistable(t)) hres = HNOTATABLE; \
    else { luaH_fastseti(hvalue(t), k, val, hres); }

    #define luaH_fastseti(t,k,val,hres) \
    { Table *h = t; lua_Unsigned u = l_castS2U(k) - 1u; \
      //未超过数组大小，此时该下标为空则直接赋值
      //否则则替换
      if ((u < h->alimit)) { \
      lu_byte *tag = getArrTag(h, u); \
      if (tagisempty(*tag)) hres = ~cast_int(u); \
        else { fval2arr(h, u, tag, val); hres = HOK; }} \
      //超过数组大小，则会放到hash表，对hash表长度取模
      else { hres = luaH_psetint(h, k, val); }}
    ```  
  + luaV_finishset，如果luaV_fastseti和luaV_fastset没有找到位置，则调用luaV_finishset：  
    ```c  
    void luaV_finishset (lua_State *L, const TValue *t, TValue *key, TValue *val, int hres) {
      int loop;  /* counter to avoid infinite loops */
      for (loop = 0; loop < MAXTAGLOOP; loop++) {
        const TValue *tm;  /* '__newindex' metamethod */
        if (hres != HNOTATABLE) {  /* is 't' a table? */
          Table *h = hvalue(t);  /* save 't' table */
          tm = fasttm(L, h->metatable, TM_NEWINDEX);  /* get metamethod */
          //没有newindex的元方法
          if (tm == NULL) {  /* no metamethod? */
            luaH_finishset(L, h, key, val, hres);  /* set new value */
            invalidateTMcache(h);
            luaC_barrierback(L, obj2gco(h), val);
            return;
          }
          /* else will try the metamethod */
        }
        else {  /* not a table; check metamethod */
          tm = luaT_gettmbyobj(L, t, TM_NEWINDEX);
          if (l_unlikely(notm(tm)))
            luaG_typeerror(L, t, "index");
        }
        /* try the metamethod */
        //元方法是个函数则调用对应的函数
        if (ttisfunction(tm)) {
          luaT_callTM(L, tm, t, key, val);
          return;
        }
        //元方法不是函数则把元方法当作一个table继续
        t = tm;  /* else repeat assignment over 'tm' */
        luaV_fastset(t, key, val, hres, luaH_pset);
        if (hres == HOK)
          return;  /* done */
        /* else 'return luaV_finishset(L, t, key, val, slot)' (loop) */
      }
      luaG_runerror(L, "'__newindex' chain too long; possible loop");
    }

    void luaH_finishset (lua_State *L, Table *t, const TValue *key, TValue *value, int hres) {
      lua_assert(hres != HOK);
      if (hres == HNOTFOUND) {
        luaH_newkey(L, t, key, value);
      }
      else if (hres > 0) {  /* regular Node? */
        setobj2t(L, gval(gnode(t, hres - HFIRSTNODE)), value);
      }
      else {  /* array entry */
        hres = ~hres;  /* real index */
        obj2arr(t, hres, value);
      }
    }
    ```
  + luaH_newkey：  
    ```c
    //如果key是int类型，且在数组大小限制内，则在这之前赋值已经完成，到这一步则是因为
    //key为非int类型或者key为int类型但超过数组限制从而被分配到hash部分。
    static void luaH_newkey (lua_State *L, Table *t, const TValue *key,TValue *value) 
    {
      Node *mp;
      TValue aux;
      if (l_unlikely(ttisnil(key)))
        luaG_runerror(L, "table index is nil");
      else if (ttisfloat(key)) 
      {
        //key为浮点数，首先尝试转成int类型
        lua_Number f = fltvalue(key);
        lua_Integer k;
        if (luaV_flttointeger(f, &k, F2Ieq)) {  /* does key fit in an integer? */
          setivalue(&aux, k);
          key = &aux;  /* insert it as an integer */
        }
        else if (l_unlikely(luai_numisnan(f)))
          luaG_runerror(L, "table index is NaN");
      }
      if (ttisnil(value))
        return;  /* do not insert nil values */
      //获取key对应的node
      mp = mainpositionTV(t, key);
      if (!isempty(gval(mp)) || isdummy(t)) {  /* main position is taken? */
        Node *othern;
        Node *f = getfreepos(t);  /* get a free place */
        if (f == NULL) {  /* cannot find a free place? */
          rehash(L, t, key);  /* grow table */
          /* whatever called 'newkey' takes care of TM cache */
          luaH_set(L, t, key, value);  /* insert key into grown table */
          return;
        }
        lua_assert(!isdummy(t));
        //获取当前位置的node的key本该所在的位置
        othern = mainpositionfromnode(t, mp);
        //如果不相等那就是其他key之前就已经发生过碰撞被分配到这里
        if (othern != mp) {  /* is colliding node out of its main position? */
          /* yes; move colliding node into free position */
          //将原来的挪到空闲位置，并将传入的key放入所对应的node里面
          while (othern + gnext(othern) != mp)  /* find previous */
            othern += gnext(othern);
          gnext(othern) = cast_int(f - othern);  /* rechain to point to 'f' */
          *f = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
          if (gnext(mp) != 0) {
            gnext(f) += cast_int(mp - f);  /* correct 'next' */
            gnext(mp) = 0;  /* now 'mp' is free */
          }
          setempty(gval(mp));
        }
        else {  /* colliding node is in its own main position */
          /* new node will go into free position */
          //赋值到找到的空闲位置并且将空闲位置连接起来
          if (gnext(mp) != 0)
            gnext(f) = cast_int((mp + gnext(mp)) - f);  /* chain new position */
          else lua_assert(gnext(f) == 0);
          gnext(mp) = cast_int(f - mp);
          mp = f;
        }
      }
      //没有发生冲突且table的lsizenode不为0，则找到对应位置直接赋值
      setnodekey(L, mp, key);
      luaC_barrierback(L, obj2gco(t), key);
      lua_assert(isempty(gval(mp)));
      setobj2t(L, gval(mp), value);
    } 
    ```
  + rehash：  
    ```c
    static void rehash (lua_State *L, Table *t, const TValue *ek) 
    {
      unsigned int asize;  /* optimal size for array part */
      unsigned int na;  /* number of keys in the array part */
      //nums[i]记录的值是key的大小在2^(i - 1)到2^i的数量
      unsigned int nums[MAXABITS + 1];
      int i;
      unsigned totaluse;
      for (i = 0; i <= MAXABITS; i++) nums[i] = 0;  /* reset counts */
      setlimittosize(t);
      //计算数组部分数据分布
      na = numusearray(t, nums);  /* count keys in array part */
      totaluse = na;  /* all those keys are integer keys */
      //计算hash部分数据分布
      totaluse += numusehash(t, nums, &na);  /* count keys in hash part */
      /* count extra key */
      if (ttisinteger(ek))
        na += countint(ivalue(ek), nums);
      totaluse++;
      /* compute new size for array part */
      //计算新的数组部分大小
      asize = computesizes(nums, &na);
      /* resize the table to new computed sizes */
      //重新分配数组以及hash表的大小
      luaH_resize(L, t, asize, totaluse - na);
    }

    //计算数组部分大小，规则是找到最大且数量超过一半的
    static unsigned computesizes (unsigned nums[], unsigned *pna) 
    {
      int i;
      unsigned int twotoi;  /* 2^i (candidate for optimal size) */
      unsigned int a = 0;  /* number of elements smaller than 2^i */
      unsigned int na = 0;  /* number of elements to go to array part */
      unsigned int optimal = 0;  /* optimal size for array part */
      /* loop while keys can fill more than half of total size */
      for (i = 0, twotoi = 1;
          twotoi > 0 && *pna > twotoi / 2;
          i++, twotoi *= 2) {
        a += nums[i];
        if (a > twotoi/2) {  /* more than half elements present? */
          optimal = twotoi;  /* optimal size (till now) */
          na = a;  /* all elements up to 'optimal' will go to array part */
        }
      }
      lua_assert((optimal == 0 || optimal / 2 < na) && na <= optimal);
      *pna = na;
      return optimal;
    }
    ```
  