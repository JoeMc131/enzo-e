// See LICENSE_CELLO file for license and copyright information

/// @file     data_FluxData.cpp
/// @author   James Bordner (jobordner@ucsd.edu)
/// @date     2020-03-30
/// @brief    Implementation of the FluxData class

#include "data.hpp"

// #define DEBUG_REFRESH

//======================================================================

void FluxData::allocate
( int nx, int ny, int nz,
  std::vector<int> field_list,
  std::vector<int> * cx_list,
  std::vector<int> * cy_list,
  std::vector<int> * cz_list)
{
  field_list_ = field_list;
  int nf = field_list.size();
  block_fluxes_.resize(6*nf,nullptr);
  neighbor_fluxes_.resize(6*nf,nullptr);

  const int ixm = -1;
  const int ixp = +1;
  const int iym = (ny == 1) ? 0 : -1;
  const int iyp = (ny == 1) ? 0 : +1;
  const int izm = (nz == 1) ? 0 : -1;
  const int izp = (nz == 1) ? 0 : +1;

  const int ix32[3][2] = { {-1, +1},  {0,  0}, {0,  0} };
  const int iy32[3][2] = { { 0,  0}, {-1, +1}, {0,  0} };
  const int iz32[3][2] = { { 0,  0},  {0, 0}, {-1, +1} };
  
  for (int i_f=0; i_f<nf; i_f++) {
        
    const int index_field = field_list[i_f];

    const int cx = cx_list ? (*cx_list)[i_f] : 0;
    const int cy = cy_list ? (*cy_list)[i_f] : 0;
    const int cz = cz_list ? (*cz_list)[i_f] : 0;

    for (int axis=0; axis<3; axis++) {
      for (int face=0; face<2; face++) {
        int ix = ix32[axis][face];
        int iy = iy32[axis][face];
        int iz = iz32[axis][face];
        FaceFluxes * ff;
        ff = new FaceFluxes
          (Face(ix,iy,iz,axis,face),index_field,nx,ny,nz,cx,cy,cz);
        ff->allocate();
        block_fluxes_[index_(axis,face,i_f)] = ff;
        ff = new FaceFluxes
          (Face(ix,iy,iz,axis,face),index_field,nx,ny,nz,cx,cy,cz);
        ff->allocate();
        neighbor_fluxes_[index_(axis,face,i_f)] = ff;
      }
    }
  }
}

//----------------------------------------------------------------------

void FluxData::deallocate()
{
  for (int i=0; i<block_fluxes_.size(); i++) {
    delete block_fluxes_[i];
  }
  block_fluxes_.clear();
  for (int i=0; i<neighbor_fluxes_.size(); i++) {
    delete neighbor_fluxes_[i];
  }
  neighbor_fluxes_.clear();
}

//----------------------------------------------------------------------

int FluxData::data_size () const
{
#ifdef DEBUG_REFRESH
  CkPrintf ("%d DEBUG_REFRESH FluxData::data_size()\n",CkMyPe());
#endif  
  int size = 0;

  size += sizeof(int);
  for (int i=0; i<block_fluxes_.size(); i++) {
    FaceFluxes * face_fluxes = block_fluxes_[i];
    int n = (face_fluxes==nullptr) ? 0 : face_fluxes->data_size();
    size += sizeof(int);
    size += n;
  }

  size += sizeof(int);
  for (int i=0; i<neighbor_fluxes_.size(); i++) {
    FaceFluxes * face_fluxes = neighbor_fluxes_[i];
    int n = (face_fluxes==nullptr) ? 0 : face_fluxes->data_size();
    size += sizeof(int);
    size += n;
  }

  size += sizeof(int);
  const int n = (field_list_.size());
  size += n*sizeof(int);
  
  return size;
}

//----------------------------------------------------------------------

char * FluxData::save_data (char * buffer) const
{
#ifdef DEBUG_REFRESH
  CkPrintf ("%d DEBUG_REFRESH FluxData::save_data()\n",CkMyPe());
#endif  
  union {
    int  * pi;
    char * pc;
  };

  pc = (char *) buffer;

  (*pi++) = block_fluxes_.size();

  for (int i=0; i<block_fluxes_.size(); i++) {
    FaceFluxes * face_fluxes = block_fluxes_[i];
    int n = (face_fluxes==nullptr) ? 0 : face_fluxes->data_size();
    (*pi++) = n;
    pc = face_fluxes->save_data(pc);
  }

  (*pi++) = neighbor_fluxes_.size();

  for (int i=0; i<neighbor_fluxes_.size(); i++) {
    FaceFluxes * face_fluxes = neighbor_fluxes_[i];
    int n = (face_fluxes==nullptr) ? 0 : face_fluxes->data_size();
    (*pi++) = n;
    pc = face_fluxes->save_data(pc);
  }

  (*pi++) = field_list_.size();
  
  for (int i=0; i<field_list_.size(); i++) {
    (*pi++) = field_list_[i];
  }
  
  ASSERT2("FluxData::save_data()",
	  "Buffer has size %ld but expecting size %d",
	  (pc-buffer),data_size(),
	  ((pc-buffer) == data_size()));

  return pc;
}

//----------------------------------------------------------------------

char * FluxData::load_data (char * buffer)
{
#ifdef DEBUG_REFRESH
  CkPrintf ("%d DEBUG_REFRESH FluxData::load_data()\n",CkMyPe());
#endif  
  union {
    int  * pi;
    char * pc;
  };

  pc = (char *) buffer;

  // block_fluxes_

  block_fluxes_.resize(*pi++);

  for (int i=0; i<block_fluxes_.size(); i++) {
    int n = (*pi++);
    FaceFluxes * face_fluxes = (n==0) ? nullptr : new FaceFluxes;
    block_fluxes_[i] = face_fluxes;
    if (face_fluxes != nullptr) {
      pc = face_fluxes->load_data(pc);
    }
  }

  // neighbor_fluxes_

  neighbor_fluxes_.resize(*pi++);

  for (int i=0; i<neighbor_fluxes_.size(); i++) {
    int n = (*pi++);
    FaceFluxes * face_fluxes = (n==0) ? nullptr : new FaceFluxes;
    neighbor_fluxes_[i] = face_fluxes;
    if (face_fluxes != nullptr) {
      pc = face_fluxes->load_data(pc);
    }
  }

  // field_list_

  int n = (*pi++);
  field_list_.resize(n);
  for (int i=0; i<field_list_.size(); i++) {
    field_list_[i] = (*pi++);
  }

  ASSERT2("FluxData::load_data()",
	  "Buffer has size %ld but expecting size %d",
	  (pc-buffer),data_size(),
	  ((pc-buffer) == data_size()));

  return pc;
}

//----------------------------------------------------------------------
  
