/*
 * Different version of the calculator which parses the command line arguments.
 * To do this the argv[] strings are first written to a ostringstream then
 * a istringstream is constructed with the string from the ostringstream and
 * fed to the lexer.
 */


// this defines an anonymous enum containing parser tokens

#include <stdio.h>
#include <malloc.h>
#include <fstream>
#include <sstream>
#include <antlr/AST.hpp>
#include "ncoParserTokenTypes.hpp"
#include "libnco++.hh"
#include "ncoLexer.hpp"
#include "ncoParser.hpp"
#include "ncoTree.hpp"
#include <antlr/CharStreamException.hpp>
#include <antlr/TokenStreamException.hpp>
#include "sdo_utl.hh" // SDO stand-alone utilities: dbg/err/wrn_prn()




//forward declaration

int           /* Evaluate expressions -execute nb - contains static members*/
ncap_mpi_exe(
std::vector< std::vector<RefAST> > &all_ast_vtr,
ncoTree** wlk_ptr_in,
int nbr_wlk_in)
{

int idx;
int jdx;
int kdx;
int mdx;
int sz;
int nbr_sz;

static int nbr_wlk; //same as number of threads
static ncoTree** wlk_ptr;

 ncoTree* wlk_lcl;

 std::vector<RefAST> inn_vtr;

// Initialize statics then exit
 if( nbr_wlk_in > 0) {
   nbr_wlk=nbr_wlk_in;
   wlk_ptr=wlk_ptr_in;
   return 2;
 }

 //Set all symbol table refs to ntl_scn=false;
 for(idx=0 ; idx< nbr_wlk ; idx++)
   wlk_ptr[idx]->prs_arg->ntl_scn=False;
  


 // Each block has two lists
 // The first list is of the expressions that contain Lvalues which 
 // are NOT defined in Output (nb this also applies to RAM vars)
 // The second list if of expressions that have all Lvalues defined in
 // output.


        
 for(idx=0 ; idx<(int)all_ast_vtr.size();idx+=2){

   // even block 
   for(jdx=0 ; jdx< (int)all_ast_vtr[idx].size();jdx++)
      (void)wlk_ptr[0]->statements(all_ast_vtr[idx][jdx]); 

   nbr_sz=(int)all_ast_vtr[idx+1].size(); 
   // odd block
   if(nbr_sz==0) continue;
   if(nbr_sz==1) {
     (void)wlk_ptr[0]->statements(all_ast_vtr[idx+1][0]);  
     continue; 
   } 

   inn_vtr=all_ast_vtr[idx+1];


#ifdef _OPENMP
#pragma omp parallel for default(none) private(kdx,wlk_lcl) shared(wlk_ptr,idx,nbr_sz,inn_vtr) 
#endif
   for(kdx=0 ;kdx< nbr_sz; kdx++) {      
     wlk_lcl= wlk_ptr[omp_get_thread_num()];
     wlk_lcl->statements(inn_vtr[kdx]);
     //(void)wlk_ptr[omp_get_thread_num()]->statements(inn_vtr[kdx]);

   } //end OPENMP parallel loop

   // Copy all atts defined in thread in to var_vtr
   for(kdx=0; kdx<nbr_wlk; kdx++){
     NcapVarVector &lcl_vtr=wlk_ptr[kdx]->prs_arg->thr_vtr;
     if(lcl_vtr.empty())
       continue;   
     for(mdx=0 ; mdx<lcl_vtr.size() ;mdx++)
       wlk_ptr[0]->prs_arg->var_vtr.push_ow(lcl_vtr[mdx]); 
     lcl_vtr.clear();   
   }
  



 } 
 // end for idx

 return 1;
}





int parse_antlr(std::vector<prs_cls> &prs_vtr,char* fl_spt_usr,char *cmd_ln_sng)
{
  
  ANTLR_USING_NAMESPACE(std);
  ANTLR_USING_NAMESPACE(antlr);
  
  const std::string fnc_nm("parse_antlr"); // [sng] Function name

  int idx;  
  char *filename;
  
  prs_cls *prs_arg;


  istringstream *sin=NULL;
  ifstream *in=NULL;
  
  ncoLexer *lexer=NULL;
  ncoParser *parser=NULL;
  
  RefAST t,a;
  ASTFactory ast_factory;

  prs_arg=&prs_vtr[0]; 
  
  std::vector<ncoTree*> wlk_vtr;
 
  filename=strdup(fl_spt_usr);   
  
  std::vector< std::vector<RefAST> > all_ast_vtr(0);


  try {
    
    if( cmd_ln_sng ){
      sin= new  istringstream(cmd_ln_sng);
      lexer= new ncoLexer( *sin, prs_arg);
    }else {
      in=new ifstream(filename);          
      lexer= new ncoLexer( *in, prs_arg);
    }     
    
    
    lexer->setFilename(filename);
    
    parser= new ncoParser(*lexer);
    parser->setFilename(filename);
    

    parser->initializeASTFactory(ast_factory);
    parser->setASTFactory(&ast_factory);
    
    
    // Parse the input expressions
    parser->program();
    a = parser->getAST();
    t=a;

    // Print parser tree
    if(dbg_lvl_get() > 0){
      dbg_prn(fnc_nm,"Printing parser tree...");
      while( t ) {
	cout << t->toStringTree() << endl;
	t=t->getNextSibling();
      }
      dbg_prn(fnc_nm,"Parser tree printed");
    } // endif dbg
    
  }  
  
  catch (RecognitionException& pe) {
    parser->reportError(pe);
    // bomb out
    nco_exit(EXIT_FAILURE);
  }
  
  catch (TokenStreamException& te) {
    cerr << te.getMessage();
    // bomb out
    nco_exit(EXIT_FAILURE);
  }
  
  catch (CharStreamException& ce) {
    cerr << ce.getMessage();
    // bomb out
    nco_exit(EXIT_FAILURE);
  }
  
  t=a;
  
  try {   
    ncoTree* wlk_obj;    
    for(idx=0 ; idx< (int)prs_vtr.size(); idx++){
      wlk_obj=new ncoTree(&prs_vtr[idx]);  
      wlk_obj->initializeASTFactory(ast_factory);
      wlk_obj->setASTFactory(&ast_factory);
      wlk_vtr.push_back(wlk_obj); 
    }      

    // initialize static members 
    (void)ncap_mpi_exe(all_ast_vtr,&wlk_vtr[0],(int)wlk_vtr.size());
    
    std::cout<<"Num Threads="<<prs_vtr.size()<<std::endl;
    if(dbg_lvl_get() > 0) dbg_prn(fnc_nm,"Walkers initialized");
  
    wlk_vtr[0]->run_exe(t,0);


    
  }  catch(std::exception& e) {
    cerr << "exception: " << e.what() << endl;
  }	
  
  if(dbg_lvl_get() > 0) dbg_prn(fnc_nm,"Walkers completed");
  
   
  
  // delete walker pointers
  for(idx=0 ; idx<(int)wlk_vtr.size() ; idx++)
    delete wlk_vtr[idx];


  delete lexer;
  delete parser;        
  if(sin) delete sin;
  if(in) delete in;

  (void)nco_free(filename);
  
  return 1;
}



