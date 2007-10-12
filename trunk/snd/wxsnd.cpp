/*
 * wxWindows port of snd library
 *
 * Dominic Mazzoni
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/file.h"

#include "wxsnd.h"

extern "C" {

void snd_fail(char *msg)
{
    wxMessageBox(msg);
}

int wxsnd_open(char *fname, int mode)
{
  wxFile *f = new wxFile();	  
  
  if (mode == O_RDONLY)
      f->Open(fname, wxFile::read);
    else
      f->Open(fname, wxFile::read_write);
    
  if (!f->IsOpened())
    return 0;
  
  return (int)f;
}

int wxsnd_creat(char *fname, int perms)
{
  wxFile *f = new wxFile();	  
  
    f->Create(fname);
    
  if (!f->IsOpened())
    return 0;
  
  return (int)f;
}

int wxsnd_lseek(int file, int offset, int param)
{
  wxFile *f = (wxFile *)file;
  
  if (param == L_SET)
    f->Seek(offset, wxFromStart);
  else if (param == L_INCR)
    f->Seek(offset, wxFromCurrent);
  else
    f->Seek(offset, wxFromEnd);
  
  return offset;
}

int wxsnd_read(int fp, char *data, int len)
{
    wxFile *f = (wxFile *)fp;
    
    return f->Read(data, len);
}

int wxsnd_write(int fp, char *data, int len)
{
    wxFile *f = (wxFile *)fp;
    
    return f->Write(data, len);
}

int wxsnd_close(int fp)
{
    wxFile *f = (wxFile *)fp;
    
    if (f)
      delete f;
    
  return 1;
}

long wxsnd_getfilelen(int fp)
{
    wxFile *f = (wxFile *)fp;

    return f->Length();
}

};
