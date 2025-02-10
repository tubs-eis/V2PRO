/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
#ifndef CNN_NET_H
#define CNN_NET_H

#include <sys/stat.h> // mkdir
#include <cstdint>
#include <cassert>
#include <string.h> // memcpy
#include <iomanip>
#include <vector>
#include <list>
#include <math.h>
#include <iostream>
#include "base_layer.h"
#include "bif.h"


/*

  Memory layout of host data structures (BIF) used to generate binary data in most cases.
  Some mem layout is generated on the fly.

*/


namespace CNN_NET {

  class Net {
  public:
    
    // RISCV byte address
    static mm_addr_type align(mm_addr_type a, mm_addr_type alignment = 16) {
      assert(((alignment & (alignment - 1)) == 0) && "alignment needs to be a power of 2 but is not");
      return (a+alignment-1) & ~(alignment-1);
    }


    Net(const std::string &name) : 
      cnn_name(name), layers(), layer_execlist() { }

    virtual void instantiateLayers() = 0;
    
    void addLayer(CNN_LAYER::Layer *layer) {
      layers.push_back(layer);
    }

    virtual void generateLayerExecList() {
      // default: execute layers in instantiation order
      layer_execlist.clear();
      for (std::vector<CNN_LAYER::Layer>::size_type i = 0; i < layers.size(); i++) {
        if (layers[i]->produces_binary_data) {
          layer_execlist.push_back(i);
        }
      }
      if (run_layers_decoupled) {
          std::reverse(layer_execlist.begin(), layer_execlist.end());
      }
    }

    // MM static memory layout
    // weight addresses must be known before segment generation
    // segment addresses will be computed on the fly
    virtual void designMmLayoutVpro() {
      // layer output data (output of input-layer is the global cnn input)
      // dumb linear allocation: each has its private output space
      mm_addr_type mm_output_addr = memlayout_static.mm_output_base;
      for (auto layer: layers) {
        // std::cout << "designMmLayoutVpro: " << layer->getFullName() << " " << layer << " .setOutputMMAddr(" << mmAddrStr(mm_output_addr) << ")\n";
        mm_output_addr = align(mm_output_addr, 16);
        layer->setOutputMMAddr(mm_output_addr); // one addr per src layer
        mm_output_addr += layer->getOutputMMSize();
      }
      /* Ideas for space-saving memory allocation:
         - full-blown register allocation and graph coloring approaches are probably unnecessarily complicated

         - algo: three passes over all layers to keep I/O spaces together tight
           - allocate all inputs
           - allocate all outputs
           - allocate buckets for all intermediate layers
             - re-use buckets no longer required
             - if all buckets are currently used, allocate a new bucket
             - bucket tracks:
               - layers using its content as input (empty list -> bucket can be re-used)
               - maximum output size of any layer writing into this bucket
             - input and output layers always have private spaces (overwritten/read in parallel by ARM)
      */

      // absolute weight addresses will be stored in command segments -> weight addresses must be known before segment generation
      // uncached .vpro data segment
      mm_addr_type mm_weights_addr = memlayout_static.mm_weights_base;
      if (mm_output_addr > mm_weights_addr) {
        std::cout << "Static memory layout failed: mm_output " << mmAddrStr(mm_output_addr) << " overlaps mm_weights " << mmAddrStr(mm_weights_addr) << ". Increase mm_weights_base in base_net.h\n";
        exit(1);
      }
      for (auto &layer: layers) {
        mm_weights_addr = align(mm_weights_addr, 16);
        layer->setWeightsMMAddr(mm_weights_addr);
        mm_weights_addr += layer->getWeightsMMSize();
      }

      std::cout << "VPRO memory blocks: (details see init/input.cfg + exit/output.cfg)\n";
      std::cout << "  " << mmAddrStr(memlayout_static.mm_output_base) << " .. " << mmAddrStr(mm_output_addr-1) << " (" << std::setw(10) << mm_output_addr-memlayout_static.mm_output_base << " byte): CNN input + layer outputs\n";
      std::cout << "  " << mmAddrStr(memlayout_static.mm_weights_base) << " .. " << mmAddrStr(mm_weights_addr-1) << " (" << std::setw(10) << mm_weights_addr-memlayout_static.mm_weights_base << " byte): weights\n";

      assert((mm_output_addr <= 0xC0000000 && mm_weights_addr <= 0xC0000000) && "ISS DMA maps addresses >= 0x40000000 to host mem (as of 2022-11-23)");
    }

    virtual Blob* generateEisvBlob() {
      /*
        EISV blob memory layout                                size
        ------------------------------------------------------------------------------------------
        BIF::NET bnet                                      bif_net + bif_layer_offs[]
          bif_layer_offs[bnet.layer_count]
        BIF::LAYER[bnet.layers]                            variable size per element
          COMMAND_SEGMENT[command_segment_count]
        uint32_t layer_execlist[bnet.layer_execlist_count]
        
        Memory layout defined by C struct where possible, the rest stitched together manually
        Assumptions:
        - packed structs
        - same endianness, pointer arithmetic and struct mem layout on host and target
      */
      std::cout << "=================== EISV blob generation '" << cnn_name << "' ===================\n";

      unsigned int layer_exec_count = layer_execlist.size(); // number of layers executed
      assert((layer_exec_count > 0) && "layer_execlist is empty");
      std::vector<uint32_t> log_idx_to_bin_idx(layers.size()); // some frontend layers[] are not in EISV blob -> different indexing
      
      // == generate list of BIF::LAYER blobs for all layers
      std::vector<Blob*> layer_blobs; // each element encapsulates the variable-size BIF::LAYER of one layer

      for (unsigned int li = 0; li < layers.size(); li++) {
        // PRINTD("li="<< li);
        if (!layers[li]->produces_binary_data)
          continue;

        std::cout << "== Layer " << layers[li]->getFullName() << "\n";
        std::vector<BIF::COMMAND_SEGMENT> &layer_cmd_segs = layers[li]->generateCommandSegments();
        std::cout << std::setw(8) << layers[li]->segments.size() << " segments -> "
                  << std::setw(6) << layer_cmd_segs.size() << " commands";

        int bl_memsize = sizeof(BIF::LAYER) + sizeof(BIF::COMMAND_SEGMENT)*layer_cmd_segs.size();
        bl_memsize = align(bl_memsize, 32); // align start of next LAYER struct in (variable-size) array
        Blob *bl_blob = new Blob(bl_memsize);
        BIF::LAYER *bl = (BIF::LAYER*)bl_blob->data();
        std::cout << ", " << std::setw(8) << bl_memsize << " byte total\n";

        // PRINTD("bl_blob: size " << bl_blob->size());

        // fill all fields of LAYER except command segments
        layers[li]->generateBifLayer(*bl);
        bl->command_segments_count = layer_cmd_segs.size();
        memcpy(&bl->command_segments, layer_cmd_segs.data(), sizeof(BIF::COMMAND_SEGMENT)*layer_cmd_segs.size());
        // PRINTD("li = " << li);

        log_idx_to_bin_idx[li] = layer_blobs.size(); // layers[li] is stored in bif_layer_offs[log_idx_to_bin_idx[li]]

        layer_blobs.push_back(bl_blob);
        // PRINTD("layer_blobs.size() = " << layer_blobs.size());
      }
      unsigned int layer_count = layer_blobs.size(); // number of layer data structures
      std::cout << "-- " << layers.size() << " frontend layers, " << layer_count << " layers in blob, " << layer_exec_count << " layers in execlist: ";
      for (unsigned int xli = 0; xli < layer_execlist.size(); xli++) {
        if (xli > 0) {
          std::cout << ", ";
        }
        int li = layers[layer_execlist[xli]]->number;
        std::cout << li;
        int skipped = 0;
        while (xli+1 < layer_execlist.size() && li+1==layers[layer_execlist[xli+1]]->number) {
          skipped++;
          xli++;
          li++;
        }
        if (skipped) {
          std::cout << ".." << li;
        }
      }
      std::cout << "\n";
      
      //    // append to global command segment list
      //    cmd_segs.(cmd_segs.end(), a.begin(), a.end());

      // == memory sizes of EISV blob elements
      unsigned int sz_bif_net = sizeof(BIF::NET) + sizeof(uint32_t)*layer_count; // reserve space for NET.bif_layer_offs[]
      sz_bif_net = align(sz_bif_net, 32); // align BIF:LAYER array to 32 byte
      unsigned int sz_bif_layers = 0;
      for (unsigned int li = 0; li < layer_count; li++) {
        sz_bif_layers += layer_blobs[li]->size();
      }
      int sz_layer_execlist = sizeof(uint32_t) * layer_exec_count;

      int eisv_blob_memsize = sz_bif_net + sz_bif_layers + sz_layer_execlist;

      std::cout << "  BIF::NET           : "                                        << std::setw(8) << sz_bif_net    << " byte including BIF::LAYER offsets\n";
      std::cout << "  BIF::LAYER    [" << std::setw(3) << layer_count  << "]: " << std::setw(8) << sz_bif_layers << " byte\n";
      std::cout << "  LAYER_EXECLIST[" << std::setw(3) << layer_exec_count << "]: " << std::setw(8) << sz_layer_execlist << " byte\n";// (entries: layers ";
      std::cout << "  EISV_BLOB          : " << std::setw(8) << eisv_blob_memsize << " byte, relocatable\n";

      
      // == combine all elements into one continous blob
      // base: BIF::NET
      //      eisv_blob.clear();
      eisv_blob.resize(eisv_blob_memsize);
      BIF::NET *bnet = (BIF::NET*)eisv_blob.data();
      bnet->magicword = BIF::net_magicword;
      bnet->blobsize = eisv_blob_memsize;
      bnet->reserved = 0;
      bnet->layer_execlist_count = layer_exec_count;
      bnet->layer_execlist_offs = sz_bif_net + sz_bif_layers;
      bnet->layer_count = layer_count;

      uint32_t bif_layer_offs = sz_bif_net;
      for (unsigned int li = 0; li < layer_count; li++) {
        bnet->bif_layer_offs[li] = bif_layer_offs;
        BIF::LAYER *bl = (BIF::LAYER*)(((uint8_t*)bnet) + bif_layer_offs);
        memcpy((void*)bl, layer_blobs[li]->data(), layer_blobs[li]->size());
        bif_layer_offs += layer_blobs[li]->size();
      }

      assert((bif_layer_offs == sz_bif_net + sz_bif_layers) && "Memory layout mismatch");
                                               
      // layer_execlist[layer_exec_count]
      //      uint32_t *layer_execlist = (uint32_t*)(((uint8_t*)bnet) + sz_bif_net + sz_bif_layers);
      uint32_t *lxl = (uint32_t*)((uint8_t*)bnet + sz_bif_net + sz_bif_layers);
      for (unsigned int xli = 0; xli < layer_execlist.size(); xli++) {
        lxl[xli] = log_idx_to_bin_idx[layer_execlist[xli]]; // layers[li] is stored in bif_layer_offs[log_idx_to_bin_idx[li]]
      }
      //      memcpy((uint8_t*)bnet + sz_bif_net + sz_bif_layers, layer_execlist.data(), layer_execlist.size() * sizeof(uint32_t));

      assert((mm_addr_type)eisv_blob_memsize <= memlayout_static.mm_eisvblob_size && "ERROR: eisv_blob_size > section size. Adapt linkter script and mm_eisvblob_size");
      
      // default: execute layers in instantiation order (different order required for recursive nets etc., unimplemented)
      return &eisv_blob;
    }

    const BIF::NET* getEisvBlob() {
      if (eisv_blob.empty())
        return NULL;
      else
        return (const BIF::NET*)eisv_blob.data();
    }

    
    virtual Blob* generateVproBlob() {
      // assumptions:
      // - addresses have already been assigned (designMmLayoutVpro())
      // - weights for all layers are located close together (i.e. reasonable to pack them into one continous memory region)
      // - order can be arbitrary
      // no overlap checking etc.
      
      mm_addr_type min_addr = memlayout_static.mm_weights_base;
      mm_addr_type max_addr_p1 = 0; // maximum address + 1
      for (auto layer : layers) {
        if (!layer->produces_binary_data)
          continue;

        assert((layer->getWeightsMMAddr() >= min_addr) && "Layer wants to place weights below mm_eisvblob_load_addr");
        max_addr_p1 = std::max(max_addr_p1, layer->getWeightsMMAddr() + layer->getWeightsMMSize());
      }

      weights_blob.clear();
      if (min_addr < max_addr_p1) {
        mm_addr_type memsize = max_addr_p1 - min_addr;
        weights_blob.resize(memsize);
        uint8_t *mem = (uint8_t*)weights_blob.data();
        
        for (unsigned int li = 0; li < layers.size(); li++) {
          if (!layers[li]->produces_binary_data)
            continue;

          memcpy(&mem[layers[li]->getWeightsMMAddr() - min_addr], layers[li]->weights_packed.data(), layers[li]->getWeightsMMSize());
        }
      }
      return &weights_blob;
    }

    virtual std::ofstream fopenw(std::string path, std::string fname, const std::string purpose) {
      if (!path.empty()) {
        /*int status = */mkdir(path.c_str(), 0777);//S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      }
      fname = path + fname;
      std::ofstream fd(fname, std::ofstream::binary | std::ofstream::out);
      if (!fd){
        std::cout << "Could not open file " << fname << " for " << purpose << " export!\n";
      }
      return fd;
    }
    
    virtual void exportEisvBlob() {
      auto fd = fopenw("generated/", "eisvblob.bin", "EISV blob");
      if (!fd) return;
      fd.write(reinterpret_cast<char*>(eisv_blob.data()), eisv_blob.size());
      fd.close();
    }

    virtual void exportVproBlob() {
      auto fd = fopenw("generated/", "vproblob.bin", "VPRO blob");
      if (!fd) return;
      fd.write(reinterpret_cast<char*>(weights_blob.data()), weights_blob.size());
      fd.close();
    }

    virtual void exportLayerIoConfig(std::ofstream &fd, CNN_LAYER::Layer *l, bool in) {
      // layer descriptor parsed by iolib
      fd << "# Layer " << l->getFullName() << ": " << l->getIoStr(false, true) << l->out_dim.detailStr() << "\n";

      // iolib's file input provider: control whether a layer's output (i.e. CNN input) is preloaded from file before CNN execution
      fd << "# ";
      if (!(run_layers_decoupled || getSimInputActiveLayer(*l)))
        fd << "!";
      fd << "file load " << l->getFullName() << ": '" << getSimInputFilenameLayer(*l) << "' format ";
      if (file_format_with_garbage) {
        fd << l->out_dim.mmStr() << " ";
      } else {
        fd << l->out_dim.algoStr() << " ";
      }
      if (!l->use_dynamic_shape) {
        fd << "!";
      }
      fd << "dynamic_shape\n";
      fd << "# ";

      // iolib's file output handler: control whether a layer's output (i.e. CNN result) is stored to file after CNN execution
      if (!(run_layers_decoupled || getSimOutputActiveLayer(*l)))
        fd << "!";
      fd << "file save " << l->getFullName() << ": '" << getSimOutputFilenameLayer(*l) << "' format ";
      if (file_format_with_garbage) {
        fd << l->out_dim.mmStr() << "\n";
      } else {
        fd << l->out_dim.algoStr() << "\n";
      }

      // ISS: control loading (input.cfg) of CNN input and storing (output.cfg) of CNN results
      if (!(run_layers_decoupled || (in && getSimInputActiveLayer(*l)) || (!in && getSimOutputActiveLayer(*l))))
        fd << "# ";
      fd << (in ? getSimInputFilenameLayer(*l) : getSimOutputFilenameLayer(*l)) << " " << mmAddrStr(l->out_dim.mm.channel_base[0]) << " "; // << " -1"; // -1: use whole file
      // tell simulator to skip garbage bytes in MM right and below image
      if (file_format_with_garbage) {
        fd << (l->out_dim.ch*l->out_dim.mm.ch_size);
      } else {
        fd << (2*l->out_dim.x*l->out_dim.y*l->out_dim.ch);
        if (l->out_dim.x != (int)l->out_dim.mm.x) {
          fd << " " << (2*l->out_dim.x) << " " << (2*(l->out_dim.mm.x - l->out_dim.x)); // a b: skip b bytes in MM every a bytes
        }
        if (l->out_dim.y != (int)l->out_dim.mm.y) {
            fd << " " << (2*l->out_dim.y*l->out_dim.x) << " " << (2*(l->out_dim.mm.y - l->out_dim.y)*l->out_dim.mm.x);
        }
      }
      fd << "\n";

      // individual channels, not used by any automated processing
      for (std::vector<CNN_LAYER::Layer>::size_type ch = 0; ch < l->out_dim.mm.channel_base.size(); ch++) {
        if (!((in && getSimInputActiveChannel(*l, ch)) || (!in && getSimOutputActiveChannel(*l, ch))))
          fd << "# ";
        fd << (in ? getSimInputFilenameChannel(*l, ch) : getSimOutputFilenameChannel(*l, ch)) << " " << mmAddrStr(l->out_dim.mm.channel_base[ch]) << " ";
        if (file_format_with_garbage) {
          fd << l->out_dim.mm.ch_size;
        } else {
          fd << (2*l->out_dim.x*l->out_dim.y);
          if (l->out_dim.x != (int)l->out_dim.mm.x) {
            fd << " " << (2*l->out_dim.x) << " " << (2*(l->out_dim.mm.x - l->out_dim.x)); // a b: skip b bytes in MM every a bytes
          }
        }
        fd << "\n";
      }
    }

    virtual void exportSimInputConfig() {
      // ISS: empty lines not allowed, all addresses in decimal (as of 2022-11-27)
      auto fd = fopenw("init/", "input.cfg", "ISS input config");
      if (!fd) return;
      fd << "# ISS input memory map for " << cnn_name << "\n";
      fd << "# Auto-generated by netgen\n";
      fd << "# Do not edit this file, overwrite getSimInput*() functions in derived Net class instead\n";
      fd << "# Notes:\n";
      fd << "# - shapes are generally specified in whc order in cnn_converter, but actual memory layout in tensorflow notation is chw\n"; // TODO: change cnn_converter to consistently use tensorflow's notation representing actual memory layout
      fd << "# - '!' denotes 'not' in e.g. file load/save and dynamic_shape context\n";
      fd << "#\n";

      mm_addr_type addr;

      addr = memlayout_static.mm_eisvblob_load_addr;
      assert(((addr & (32-1)) == 0) && "mm_eisvblob_load_addr must be 32 bit aligned");
      fd << "# == CNN descriptor: net, layers, commands (EISV, cached memory)\n";
      fd << "../generated/eisvblob.bin " << mmAddrStr(addr) << "\n";
      fd << "#\n";

      addr = memlayout_static.mm_weights_base;
      fd << "# == weights (VPRO, uncached memory)\n";
      fd << "../generated/vproblob.bin " << mmAddrStr(addr) << "\n";
      fd << "#\n";

      fd << "# == Input image(s) (VPRO, uncached memory)\n";
      addr = memlayout_static.mm_output_base; // CNN input is the output of the "input" layer
      for (std::vector<CNN_LAYER::Layer>::size_type i = 0; i < layers.size(); i++) {
        exportLayerIoConfig(fd, layers[i], true);
      }
      
      fd.close();
    }

    // std bug: std::setfill('0') << std::setw(width) << i outputs "0-1" instead of "-01"
    std::string to_signed_string(int i, int width) {
      std::stringstream ss;
      if (i >= 0)
        ss << std::setfill('0') << std::setw(width) << i;
      else
        ss << "-" << std::setfill('0') << std::setw(width-1) << -i;
      return ss.str();
    }

    // input.cfg
    // filename for initialization of layer output (load CNN input and intermediate inputs, if any)
    // ISS reads/writes everything to <cwd>/../, so do the same here (2022-11-27)
    // all channels in one file (per layer)
    virtual std::string getSimInputFilenameLayer(const CNN_LAYER::Layer &layer) {
      return "../input/l" + to_signed_string(layer.number, 3) + ".bin";
    }
    virtual bool getSimInputActiveLayer(const CNN_LAYER::Layer &layer) {
      // default: one file per layer
      return layer.is_input_layer;
    }

    // one file per channel
    virtual std::string getSimInputFilenameChannel(const CNN_LAYER::Layer &layer, int ch) {
      return "../input/l" + to_signed_string(layer.number, 3) + "_ch" + to_signed_string(ch, 4) + ".bin";
    }
    virtual bool getSimInputActiveChannel(const CNN_LAYER::Layer &layer, int ch) {
      return false;
    }
    
    virtual void exportSimOutputConfig() {
      auto fd = fopenw("exit/", "output.cfg", "ISS output config");
      if (!fd) return;
      fd << "# ISS output memory map for " << cnn_name << "\n";
      fd << "# Auto-generated by netgen\n";
      fd << "# Do not edit this file, overwrite getSimOutput*() functions in derived Net class instead\n";
      fd << "# Notes:\n";
      fd << "# - shapes are generally specified in whc order in cnn_converter, but actual memory layout in tensorflow notation is chw\n"; // TODO: change cnn_converter to consistently use tensorflow's notation representing actual memory layout
      fd << "# - '!' denotes 'not' in e.g. file load/save and dynamic_shape context\n";
      fd << "#\n";

      fd << "# == Output image(s)\n";
      for (std::vector<CNN_LAYER::Layer>::size_type i = 0; i < layers.size(); i++) {
        exportLayerIoConfig(fd, layers[i], false);
      }
      
      fd.close();
    }

    // output.cfg
    // filename to store layer output
    // ISS reads/writes everything to <cwd>/../, so do the same here (2022-11-27)
    // all channels in one file (per layer)
    virtual std::string getSimOutputFilenameLayer(const CNN_LAYER::Layer &layer) {
      return "../sim_results/l" + to_signed_string(layer.number, 3) + ".bin";
    }
    virtual bool getSimOutputActiveLayer(const CNN_LAYER::Layer &layer) {
      // default: dump outputs and intermediate layers
      return true;
    }

    // one file per channel
    virtual std::string getSimOutputFilenameChannel(const CNN_LAYER::Layer &layer, int ch) {
      return "../sim_results/l" + to_signed_string(layer.number, 3) + "_ch" + to_signed_string(ch, 4) + ".bin";
    }
    virtual bool getSimOutputActiveChannel(const CNN_LAYER::Layer &layer, int ch) {
      return false;
    }

    // do everything this net can do
    virtual void generateNet() {
      if (run_layers_decoupled) {
        std::cout << "\
!!! run_layers_decoupled is active !!!\n\
Independent execution of all layers in a net, useful to quickly identify working and non-working layers\n\
- all layer outputs are preloaded with reference data which must be copied to input/ from:\n\
   - ref_fixed/: run nn_quant with evaluate_all_outputs=True\n\
   - sim_results/\n\
   - emu_results/\n\
- layers are executed in reverse order for decoupled execution\n\
- dump all layer outputs\n\
- compare results vs. reference\n";
      }

      instantiateLayers();

      designMmLayoutVpro();

      generateLayerExecList();

      if (run_layers_decoupled) {
        layers[layer_execlist.front()]->first_layer_producing_output = true;
        layers[layer_execlist.back()]->last_layer_using_input = true;
      } else {
        // find last layer in execution order that uses a CNN input
        for (int eli = (int)layer_execlist.size()-1; eli >= 0; eli--) {
          CNN_LAYER::Layer *l = layers[layer_execlist[eli]];
          for (CNN_LAYER::Layer *sl: l->src_layers) {
            if (sl->is_transient_input_layer()) {
            // if (sl->is_input_layer) {
              l->last_layer_using_input = true;
              eli = -1; // also break outer loop
              break;
            }
          }
        }

        // find first layer in execution order that writes to a CNN output
        for (unsigned eli = 0; eli < layer_execlist.size(); eli++) {
          CNN_LAYER::Layer *l = layers[layer_execlist[eli]];
          if (l->out_is_result) {
            l->first_layer_producing_output = true;
            break;
          }
        }
      }

      std::cout << getLayersInfoText();

      // sanity check: at least one executed layer must consume input, at least one must produce output (otherwise handshake with host will hang)
      int nlast = 0;
      int nfirst = 0;      
      for (unsigned eli = 0; eli < layer_execlist.size(); eli++) {
        CNN_LAYER::Layer *l = layers[layer_execlist[eli]];
        if (l->last_layer_using_input)
          nlast++;
        if (l->first_layer_producing_output)
          nfirst++;
      }
      if (nlast != 1) {
        std::cout << "!! WARNING Expecting exactly one last_layer_using_input, layer_execlist contains " << nlast << ". Make sure at least one layer reading from an input layer is executed. Handshake EIS-V <-> ARM and thus streaming will fail.\n";
      }
      if (nfirst != 1) {
        std::cout << "!! WARNING Expecting exactly one first_layer_producing_output, layer_execlist contains " << nfirst << ". Make sure least one layer marked as output (Layer.out_is_result) is executed. Handshake EIS-V <-> ARM and thus streaming will fail.\n";
      }

      // segments contain absolute mm addresses (.vpro section) -> define mm layout before segment generation
      generateVproBlob();
      generateEisvBlob();

      exportEisvBlob();
      exportVproBlob();

      exportLayersText();
      exportSegmentsText();
      exportLaneUsageText();
      exportCommandsText();
      exportSimInputConfig();
      exportSimOutputConfig();
    }

    virtual std::string getLayersInfoText() {
      std::stringstream ss;
      ss << "=================== Frontend-dump '" << cnn_name << "' ===================\n";
      for (auto layer: layers) {
        ss << layer->getLayerInfoText();
      }
      return ss.str();
    }

    virtual void exportLayersText() {
      auto fd = fopenw("generated/", "layers.txt", "layer");
      if (!fd) return;

      for (unsigned int li = 0; li < layers.size(); li++) {
        if (!layers[li]->produces_binary_data)
          continue;
        fd << "LAYER " << layers[li]->getFullName() << ", " << layers[li]->getLayerTypeName() << "\n";

        BIF::LAYER bl;
        layers[li]->generateBifLayer(bl);
        fd << bl.to_char(true);
      }
      fd.close();
    }

    virtual void exportSegmentsText() {
      auto fd = fopenw("generated/", "segments.txt", "segment");
      if (!fd) return;
      fd << "# Format: <linear segment number> (<mapped processing element location>): Dummy/First/Last xy(<image location>) <input address(es) and row stride(s)>, <output address and row stride>, <padding>\n\n";
      for (unsigned int li = 0; li < layers.size(); li++) {
        if (!layers[li]->produces_binary_data)
          continue;
        fd << "LAYER " << layers[li]->getFullName() << ", " << layers[li]->getLayerTypeName() << ": " << layers[li]->segments.size() << " segments\n";
        fd << layers[li]->getSegmentsShortString();
      }
      fd.close();
    }

    virtual void exportLaneUsageText() {
      auto fd = fopenw("generated/", "lane_usage.txt", "lane_usage");
      if (!fd) return;
      fd << "Mapping of segments to lanes\nLegend:\nF: isFirst\nL: isLast\n1: isFirst && isLast\nx: !isFirst && !isLast\n-: dummy\n\n";
      for (unsigned int li = 0; li < layers.size(); li++) {
        if (!layers[li]->produces_binary_data)
          continue;
        int segs_per_set = VPRO_CFG::parallel_Lanes * layers[li]->parallel_outchannels_per_lane;
        fd << "LAYER " << layers[li]->getFullName() << ", " << layers[li]->getLayerTypeName() << ": " << layers[li]->segments.size() << " segments in " << (layers[li]->segments.size() / segs_per_set) << " sets (" << layers[li]->parallel_outchannels_per_lane << " parallel_outchannels_per_lane, " << VPRO_CFG::CLUSTERS << "c" << VPRO_CFG::UNITS << "u" << VPRO_CFG::LANES << "l)\n";
        fd << layers[li]->getLaneUsageString();
      }
      fd.close();
    }

    virtual void exportCommandsText() {
      auto fd = fopenw("generated/", "commands.txt", "command");
      if (!fd) return;
      for (unsigned int li = 0; li < layers.size(); li++) {
        if (!layers[li]->produces_binary_data)
          continue;
        fd << "LAYER " << layers[li]->getFullName() << ", " << layers[li]->getLayerTypeName() << ": " << layers[li]->commands.size() << " commands\n";
        fd << layers[li]->getCommandsString();
      }
      fd.close();
    }
    
    
  public:
    std::string cnn_name;
    std::string data_dir{};
    bool file_format_with_garbage = true; // files contain same garbage right and below image (out_dim.mm.(x|y) - out_dim.(x|y)) as MM
    // FIXME make false the default when supported by emu; already supported by ISS

#ifndef RUN_LAYERS_DECOUPLED
#define RUN_LAYERS_DECOUPLED false
#endif
    bool run_layers_decoupled{RUN_LAYERS_DECOUPLED};

    //  protected:
    std::vector<CNN_LAYER::Layer*> layers;
    std::vector<int> layer_execlist; // index into layers[]

    struct {
      mm_addr_type mm_eisvblob_load_addr{ 0x06000000 };              // manually synchronized with linker script; must be 32 byte aligned
      mm_addr_type mm_eisvblob_size     { 0x80000000 - 0x06000000 }; // manually synchronized with linker script
      mm_addr_type mm_output_base       { 0x81000000 };              // 496 MB reserved (until 0xA0000000 -> Weights Base)
      mm_addr_type mm_weights_base      { 0xA0000000 };              // 512 MB reserved (until 0xC0000000 -> IO Area)
    } memlayout_static;

    
    Blob eisv_blob;
    Blob weights_blob;

  }; // class Net

} // namespace CNN_NET

#endif // CNN_NET_H

