/* $Header$ */

/* Purpose: String utilities */

/* Copyright (C) 1995--2016 Charlie Zender
   This file is part of NCO, the netCDF Operators. NCO is free software.
   You may redistribute and/or modify NCO under the terms of the 
   GNU General Public License (GPL) Version 3 with exceptions described in the LICENSE file */

#include "nco_sng_utl.h" /* String utilities */

char* nco_mta_dlm="#"; /* [sng] Multi-argument delimiter */
const char* nco_mta_sub_dlm=","; /* [sng] Multi-argument sub-delimiter */

kvm_sct /* O [kvm_sct] key-value pair*/
nco_sng2kvm /* [fnc] Convert string to key-value pair */
(const char *args) /* I [sng] Input string argument with equal sign connecting key & value */
{
/* Implementation: parsing args so they can be sent to kvm (fake kvm here) as key-value pair
 *
 * Example 1: ... --gaa a=1 ... should be exported as kvm.key = a; kvm.value = 1
 * Example 2: ... --gaa "a;b;c"=1 should be exported as kvm[0].key="a", kvm[1].key="b", kvm[2].key="c"
 *          and kvm[@] = 1 (the ";" will be parsed by caller). 
 *
 * IMPORTANT: free() fake_kvm after using string_to_kvm. */

  int arg_index = 0;
  kvm_sct kvm;

  kvm.val = NULL;

  for(char* char_token = strtok((char*)args, "="); char_token; char_token = strtok(NULL, "=")){
    /* Use memcpy() because strdup() is not standard C library function and memcpy() is faster than strcpy() */
    char_token = nco_sng_strip(char_token);

    if(arg_index == 0){

      kvm.key = (char*)malloc(strlen(char_token) + 1);
      if(kvm.key){memcpy(kvm.key, char_token, strlen(char_token) + 1);}

    }else if(arg_index == 1){

      kvm.val = (char*)malloc(strlen(char_token) + 1);
      if(kvm.val){memcpy(kvm.val, char_token, strlen(char_token) + 1);}

    }else{break;} //end if
    // Next token
    arg_index++;
  } // end of loop

  // If malloc() cannot allocate sufficient memory, either key or value would be NULL; print error message and not quit.
  if(!kvm.key || !kvm.val){
    (void)fprintf(stderr, "%s: ERROR system does not have sufficient memory.\n", nco_prg_nm_get());
    nco_exit(EXIT_FAILURE);
  }
  return kvm;
}

char * /* O [sng] Stripped-string */
nco_sng_strip /* [fnc] Strip leading and trailing white space */
(char *sng) /* I/O [sng] String to strip */
{
  /* fxm: seems not working for \n??? */
  char *srt=sng;
  while(isspace(*srt)) srt++;
  size_t end=strlen(srt);
  if(srt != sng){
    memmove(sng,srt,end);
    sng[end]='\0';
  } /* endif */
  while(isblank(*(sng+end-1L))) end--;
  sng[end]='\0';
  return sng;
} /* end nco_sng_strip() */

kvm_sct * /* O [sct] Pointer to free'd kvm list */
nco_kvm_lst_free /* [fnc] Relinquish dynamic memory from list of kvm structures */
(kvm_sct *kvm, /* I/O [sct] List of kvm structures */
 const int kvm_nbr) /* I [nbr] Number of kvm structures */
{
  /* Purpose: Relinquish dynamic memory from list of kvm structures
     End of list is indicated by NULL in key slot */
  for(int kvm_idx=0;kvm_idx<kvm_nbr;kvm_idx++){
    /* Check pointers' nullity */
    if(kvm[kvm_idx].key){kvm[kvm_idx].key=(char *)nco_free(kvm[kvm_idx].key);}
    if(kvm[kvm_idx].val){kvm[kvm_idx].val=(char *)nco_free(kvm[kvm_idx].val);}
  } /* end for */
  if(kvm) kvm=(kvm_sct *)nco_free(kvm);
  return kvm;
} /* end nco_kvm_lst_free() */

void
nco_kvm_prn(kvm_sct kvm)
{
  if(kvm.key) (void)fprintf(stdout,"%s = %s\n",kvm.key,kvm.val); else return;
} /* end nco_kvm_prn() */

char *nco_strip_backslash(char* args)
{
    char* backslash_pos=strchr(args, '\\');
    strcpy(backslash_pos, nco_mta_dlm);

    return args;
}

char ** /* O [pointer to sngs] group of splitted sngs*/
nco_string_split /* [fnc] split the string by delimiter */
(const char *restrict source, /* I [sng] the source string */
const char* delimiter) /* I [char] the delimiter*/
{
    /* Use to split the string into a double character pointer, which each sencondary pointer represents
     * the string after splitting.
     * Example: a, b=1 will be split into *<a> = "a" *<b> = "b=1" with a delimiter of SUBDELIMITER 
     * Remember to free after calling this function. */
    char** sng_fnl=NULL, *temp=strdup(source);
    size_t counter=nco_count_blocks(source, (char*)delimiter), index=0;    

    if(!strstr(temp, delimiter)){ //special case for one single argument
      sng_fnl=(char**)malloc(sizeof(char*));
      sng_fnl[0]=temp;
      return sng_fnl;
    }

    sng_fnl=(char**)malloc(sizeof(char*) * counter);
    if(sng_fnl){
        for(char *token=strtok(temp, delimiter); token; token=strtok(NULL, delimiter)){
            // const char* find = strchr(sng_fnl[index - 1], '\\');
            // if(index > 0 && find && find - sng_fnl[index - 1] + 1 == strlen(sng_fnl[index - 1])){
            //     sng_fnl[index - 1] = nco_strip_backslash(sng_fnl[index-1]);
            //     strcat(sng_fnl[index - 1], token);
            // }
            // else
                sng_fnl[index ++] = strdup(token);
        } //end for
        free(temp);
    }else{return NULL;} //end if
    return sng_fnl;
}

int /* O [int] the boolean for the checking result */
nco_input_check /* [fnc] check whether the input is legal and give feedback accordingly. */
(const char *restrict args) /* O [sng] input arguments */
{
    /* Use to check the syntax for the arguments.
     * If the return value is false (which means the input value is illegal) the parser will terminate the program. */
    if(!strstr(args,"=")){ //If no equal sign in arguments
        (void)fprintf(stderr,"%s: ERROR No equal sign detected \033[0m\n", nco_prg_nm_get());
        return 0;
    } //endif
    if(strstr(args,"=")==args){ //If equal sign is in the very beginning of the arguments (no key)
        (void)fprintf(stderr,"%s: ERROR No key in key-value pair.\033[0m\n", nco_prg_nm_get()); 
        return 0;
    } //endif
    if(strstr(args,"=")==args+strlen(args)-1){ //If equal sign is in the very end of the arguments
        (void)fprintf(stderr,"%s: ERROR No value in key-value pair.\033[0m\n", nco_prg_nm_get()); 
        return 0;
    } //endif
    return 1;
}

int // O [int] the number of string blocks if will be split with delimiter
nco_count_blocks // [fnc] Check number of string blocks if will be split with delimiter
(const char* args, // I [sng] the string which is going to be split
char* delimiter) // I [sng] the delimiter
{
  int sng_nbr=0;
  const char *crnt_chr=strchr(args, *(delimiter));

  while (crnt_chr) {
    if((crnt_chr-1)[0]!='\\')
        sng_nbr++;
    crnt_chr = strchr(crnt_chr+1, *(delimiter));
  }
  return sng_nbr+1;
}

void 
nco_sng_lst_free_void /* [fnc] free() string list */
(char **restrict sng_lst, /* I/O [sng] String list to free() */
 const int sng_nbr) /* I [int] Number of strings in list */
{
    /* Use to free the double character pointer, and set the pointer to NULL */
    for(int index=0;index<sng_nbr;index++){free(sng_lst[index]);}
    free(sng_lst);
    sng_lst = NULL;
}

kvm_sct* /* O [kvm_sct] the pointer to the first kvm structure */
nco_arg_mlt_prs /* [fnc] main parser, split the string and assign to kvm structure */
(const char *restrict args) /* I [sng] input string */
{
    /* Main parser for the argument. This will split the whole argument into key value pair and send to sng2kvm*/
    if(!args) 
      nco_exit(EXIT_FAILURE);

    char **separate_args=nco_string_split(args, (const char*)nco_mta_dlm);
    size_t counter=nco_count_blocks(args,nco_mta_dlm)*nco_count_blocks(args, (char*)nco_mta_sub_dlm); //Max number of kvm structure in this argument

    for(int index=0;index<nco_count_blocks(args,nco_mta_dlm);index++){
        if(!nco_input_check(separate_args[index]))
            nco_exit(EXIT_FAILURE);//end if
    }//end loop

    kvm_sct* kvm_set=(kvm_sct*)malloc(sizeof(kvm_sct)*(counter+1)); //kvm array intended to be returned
    counter=0;

    for(int sng_index=0;sng_index<nco_count_blocks(args,nco_mta_dlm);sng_index++){
        char *value = strdup(strstr(separate_args[sng_index], "="));
        char **individual_args = nco_string_split(separate_args[sng_index], nco_mta_sub_dlm);

        for(int sub_index=0; sub_index<nco_count_blocks(separate_args[sng_index], (char*)nco_mta_sub_dlm);sub_index++){
            char* temp_value = strdup(individual_args[sub_index]);
            if(!strstr(temp_value, "=")) 
                temp_value = strcat(temp_value, value);//end if
            kvm_sct kvm_object = nco_sng2kvm(temp_value);
            kvm_set[counter++] = kvm_object;
            free(temp_value);
        }//end inner loop
        nco_sng_lst_free_void(individual_args, nco_count_blocks(separate_args[sng_index],(char*)nco_mta_sub_dlm));
        free(value);
    }//end outer loop
    nco_sng_lst_free_void(separate_args, nco_count_blocks(args, nco_mta_dlm));
    kvm_set[counter].key=NULL; //Add an ending flag for kvm array.
    return kvm_set;
}

char * /* O [sng] Joined strings */
nco_join_sng /* [fnc] Join strings with delimiter */
(const char **restrict sng_lst, /* I [sng] List of strings being connected */
 const int sng_nbr) /* I [int] Number of strings */
{
    if(sng_nbr==1) 
        return strdup(sng_lst[0]);

    size_t word_length=0;
    size_t copy_counter=0;
    for(size_t index=0;index<sng_nbr;index++){
        word_length+=strlen(sng_lst[index])+1;
    }
    char *final_string = (char*)malloc(word_length+1);
    for(int sng_index=0;sng_index<sng_nbr;sng_index++){
        size_t temp_length=strlen(sng_lst[sng_index]);
        memcpy(final_string+copy_counter, sng_lst[sng_index], temp_length);

        if(sng_index<sng_nbr-1) 
            memcpy(final_string + copy_counter + temp_length, nco_mta_dlm, 1);
        copy_counter+=(temp_length+1);
    }
    return final_string;
}

char * nco_mlt_arg_dlm_set(const char *dlm_sng_usr)
{
  nco_mta_dlm=(char*)malloc(strlen(dlm_sng_usr) + 1);
  strcpy(nco_mta_dlm, dlm_sng_usr);
  return nco_mta_dlm;
}
