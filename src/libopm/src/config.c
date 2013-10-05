/*
 * Copyright (C) 2002  Erik Fears
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to
 *
 *       The Free Software Foundation, Inc.
 *       59 Temple Place - Suite 330
 *       Boston, MA  02111-1307, USA.
 *
 *
 */

#include "setup.h"

#include "malloc.h"
#include "config.h"
#include "inet.h"
#include "opm_error.h"
#include "opm_types.h"
#include "opm_common.h"
#include "list.h"

#ifdef STDC_HEADERS
# include <string.h>
#endif

RCSID("$Id: config.c,v 1.28 2005/07/31 09:31:10 phil Exp $");

static OPM_CONFIG_HASH_T HASH[] = {
   {OPM_CONFIG_FD_LIMIT,       OPM_TYPE_INT},
   {OPM_CONFIG_BIND_IP ,       OPM_TYPE_ADDRESS},
   {OPM_CONFIG_DNSBL_HOST,     OPM_TYPE_STRING},
   {OPM_CONFIG_TARGET_STRING,  OPM_TYPE_STRINGLIST},
   {OPM_CONFIG_SCAN_IP,        OPM_TYPE_STRING},
   {OPM_CONFIG_SCAN_PORT,      OPM_TYPE_INT},
   {OPM_CONFIG_MAX_READ,       OPM_TYPE_INT},
   {OPM_CONFIG_TIMEOUT,        OPM_TYPE_INT},
};


/* config_create
 *
 *    Create an OPM_CONFIG_T struct, set default values and return it
 *
 * Parameters:
 *    None;
 *
 * Return:
 *    Pointer to allocated OPM_CONFIG_T struct
 */

OPM_CONFIG_T *libopm_config_create()
{
   int num, i;
   OPM_CONFIG_T *ret;

   num = sizeof(HASH) / sizeof(OPM_CONFIG_HASH_T);

   ret = MyMalloc(sizeof(OPM_CONFIG_T));
   ret->vars = MyMalloc(sizeof(void *) * num);


   /* Set default config items. This in the future would be much better
      if it could set realistic defaults for each individual config item.

      OPM_TYPE_INT     = 0
      OPM_TYPE_STRING  = ""
      OPM_TYPE_ADDRESS = 0.0.0.0 
      OPM_TYPE_STRINGLIST = empty list
   */

   for(i = 0; i < num; i++)
   {
      switch(libopm_config_gettype(i))
      {
         case OPM_TYPE_INT:
            ret->vars[i] = MyMalloc(sizeof(int));
            *(int *) ret->vars[i] = 0;
            break;

         case OPM_TYPE_STRING:
            ret->vars[i] = strdup("");
            break;

         case OPM_TYPE_ADDRESS:
            ret->vars[i] = MyMalloc(sizeof(opm_sockaddr));
            memset((opm_sockaddr *) ret->vars[i], 0, sizeof(opm_sockaddr));
            break; 

         case OPM_TYPE_STRINGLIST:
            ret->vars[i] = libopm_list_create();
            break;
         default:
            ret->vars[i] = NULL;
      }
   }
   return ret;
}




/* config_free
 *
 *    Free config structure and clean up
 *
 * Parameters:
 *    config: Structure to free/cleanup
 *    
 * Return:
 *    None
 */

void libopm_config_free(OPM_CONFIG_T *config)
{
   int num, i;
   OPM_NODE_T *p, *next;
   OPM_LIST_T *list;

   num = sizeof(HASH) / sizeof(OPM_CONFIG_HASH_T);

   for(i = 0; i < num; i++)
   {
      if(config->vars[i] == NULL)
         continue;

      switch(libopm_config_gettype(i))
      {
         case OPM_TYPE_STRINGLIST:
            list = (OPM_LIST_T *) config->vars[i];
            LIST_FOREACH_SAFE(p, next, list->head)
            {
               MyFree(p->data);
               MyFree(p); 
            }
            break;
         default:
            MyFree(config->vars[i]);
            break;
      }
   }

   MyFree(config->vars);
   MyFree(config);
}




/* config_set
 *
 *    Set configuration options on config struct.
 *
 * Parameters:
 *    config: Config struct to set parameters on
 *    key:    Variable within the struct to set
 *    value:  Address of value to set 
 *
 * Return:
 *    1: Variable was set
 *    0: Some error occured
 */

OPM_ERR_T libopm_config_set(OPM_CONFIG_T *config, int key, void *value)
{

   int num;
   OPM_NODE_T *node;

   num = sizeof(HASH) / sizeof(OPM_CONFIG_HASH_T);
   
   if(key < 0 || key >= num)
      return OPM_ERR_BADKEY; /* Return appropriate error code eventually */  

   switch(libopm_config_gettype(key))
   {
      case OPM_TYPE_STRING:
         if((char *) config->vars[key] != NULL)
            MyFree(config->vars[key]);
         config->vars[key] = strdup((char *) value);
         break;

      case OPM_TYPE_INT:
         *(int *) config->vars[key] = *(int *) value;
         break;

      case OPM_TYPE_ADDRESS:
         if( inet_pton(AF_INET, (char *) value, &( ((opm_sockaddr *)config->vars[key])->sa4.sin_addr))
                  <= 0)
            return OPM_ERR_BADVALUE; /* return appropriate err code */
         break; 

      case OPM_TYPE_STRINGLIST:
         node = libopm_node_create(strdup((char *) value));
         libopm_list_add((OPM_LIST_T *) config->vars[key], node);
         break;                        

      default:
         return OPM_ERR_BADKEY; /* return appropriate err code */

   }

   return OPM_SUCCESS;

}




/* config_gettype
 *
 *    Get type of key.
 * 
 * Parameters:
 *    key: Key to get type of.
 *    
 * Return:
 *    TYPE_? of key
 */

int libopm_config_gettype(int key)
{
   int num, i;

   num = sizeof(HASH) / sizeof(OPM_CONFIG_HASH_T);

   for(i = 0; i < num; i++)
      if(HASH[i].key == key)
         return HASH[i].type;
       
   return 0;
}

/* config
 *
 *    Retrieve a specific config variable from
 *    an OPM_CONFIG_T struct. This is basically a 
 *    wrapper to extracting the variable from the
 *    array.
 *
 * Parameters:
 *    config: Config struct to extract from
 *    key:    Value to extract
 *
 * Return:
 *    -ADDRESS- to extracted value in array. This address
 *    will have to be cast on the return end to be any use.
 */

void *libopm_config(OPM_CONFIG_T *config, int key)
{
   return config->vars[key];
}
