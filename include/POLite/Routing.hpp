#ifndef Routing_hpp
#define Routing_hpp

#include <iostream>
#include <vector>
#include <array>
#include <cassert>
#include <sstream>
#include <memory>
#include <unordered_set>
#include <functional>
#include <algorithm>

#include "config.h"

struct Route;
struct Link;
struct LinkIn;
struct LinkOut;
struct Node;
struct FPGANode;
struct MailboxNode;

/*
    In the tinsel network model:
    - Down : decreasing y, goes to South link
    - Up   : increasing y, goes to North link
    - Left : decreasing x, goes to East link
    - Right : increasing x, goes to West link

    So viewed as a 2D grid, the (0,0) point is in the bottom-left corner.
     
    Routing is ordered by:
    - FPGA Up/Down (North/South)
    - FPGA Left/Right (East/West) 
    - Mailbox Up/Down (North/South)
    - Mailbox Left/Right (East/West)

    In this routing model we only use the N/S/E/W names,
    both for directions and for links.
*/

enum Direction
{
    North,
    South,
    East,
    West,
    Here
};

struct thread_id_t
{
    unsigned value;
};

struct route_id_t
{
    unsigned value;
};

template<class Tag>
struct Coord
{
    unsigned x;
    unsigned y;

    bool operator==(const Coord &o) const
    { return x==o.x && y==o.y; }
};


using MailboxCoord=Coord<MailboxNode>;
using FPGACoord=Coord<FPGANode>;

struct SystemParameters
{
    unsigned FPGAMeshXLen;
    unsigned FPGAMeshYLen;
    unsigned FPGAMeshYBits;
    unsigned FPGAMeshXBits;

    unsigned MailboxMeshXLen;
    unsigned MailboxMeshYLen;
    unsigned MailboxMeshYBits;
    unsigned MailboxMeshXBits;

    unsigned LogThreadsPerMailbox;

    FPGACoord get_fpga_coord(thread_id_t thread) const
    {
        auto tmp=thread.value >> (MailboxMeshXBits+MailboxMeshYBits+LogThreadsPerMailbox);
        auto x=tmp & ((1u<<FPGAMeshXBits)-1);
        auto y=tmp & ((1u<<FPGAMeshYBits)-1);
        assert(x<FPGAMeshXLen);
        assert(y<FPGAMeshYLen);
        return FPGACoord{x,y};
    }

    // This always returns contiguous coordinates, even if the mesh is not a binary power
    MailboxCoord get_mailbox_coord(thread_id_t thread) const
    {
        auto mx = (thread.value >> LogThreadsPerMailbox) & ((1u<<MailboxMeshXBits)-1);
        auto my = (thread.value >> (MailboxMeshXBits+LogThreadsPerMailbox)) & ((1u<<MailboxMeshYBits)-1);
        auto fx = (thread.value >> (MailboxMeshYBits+MailboxMeshXBits+LogThreadsPerMailbox)) & ((1u<<FPGAMeshXBits)-1);
        auto fy = (thread.value >> (FPGAMeshXBits+MailboxMeshYBits+MailboxMeshXBits+LogThreadsPerMailbox)) & ((1u<<FPGAMeshYBits)-1);
        assert(mx<MailboxMeshXLen);
        assert(my<MailboxMeshYLen);
        assert(fx<FPGAMeshXLen);
        assert(fy<FPGAMeshYLen);
        return MailboxCoord{fx*MailboxMeshXLen+mx,fy*MailboxMeshYLen+my};
    }
};

void initSystemParameters(SystemParameters *p, unsigned FPGAMeshXLen, unsigned FPGAMeshYLen, unsigned MailboxMeshXLen, unsigned MailboxMeshYLen)
{

    unsigned LogCoresPerMailbox = TinselLogCoresPerMailbox;
    unsigned LogThreadsPerCore = TinselLogThreadsPerCore;
    p->LogThreadsPerMailbox = LogCoresPerMailbox + LogThreadsPerCore;

    // hard-coded
    p->FPGAMeshXBits=TinselMeshXBits;
    p->FPGAMeshYBits=TinselMeshYBits;
    p->MailboxMeshXBits=TinselMailboxMeshXBits;
    p->MailboxMeshYBits=TinselMailboxMeshYBits;

    if((1u<<FPGAMeshXLen) < FPGAMeshXLen ){
        throw std::runtime_error("Not enough FPGA Mesh X bits (from hard-coded config.h).");
    }
    if((1u<<FPGAMeshYLen) < FPGAMeshYLen ){
        throw std::runtime_error("Not enough FPGA Mesh Y bits (from hard-coded config.h).");
    }
    if((1u<<MailboxMeshXLen) < MailboxMeshXLen ){
        throw std::runtime_error("Not enough mailbox Mesh X bits (from hard-coded config.h).");
    }
    if((1u<<MailboxMeshYLen) < MailboxMeshYLen ){
        throw std::runtime_error("Not enough mailbox Mesh Y bits (from hard-coded config.h).");
    }

    p->FPGAMeshXLen=FPGAMeshXLen;
    p->FPGAMeshYLen=FPGAMeshYLen;
    p->MailboxMeshXLen=MailboxMeshXLen;
    p->MailboxMeshYLen=MailboxMeshYLen;
}


template<class Tag>
Direction direction_to(Coord<Tag> from, Coord<Tag> to)
{
    if(to.y < from.y){
        return South;
    }
    if(to.y > from.y){
        return North;
    }
    if(to.x < from.x){
        return East;
    }
    if(to.x > from.x){
        return West;
    }
    return Here;
}

int direction_dx(Direction d)
{
    switch(d){
        case West: return -1;
        case East: return +1;
        default: return 0;
    }
}

int direction_dy(Direction d)
{
    switch(d){
        case North: return -1;
        case South: return +1;
        default: return 0;
    }
}

struct Route
{
    thread_id_t source_thread;
    thread_id_t dest_thread;
    Node *source_node;
    Node *sink_node;
    std::vector<LinkOut*> path;
};

struct Link
{
protected:
    Link(const Node *_node, const std::string &_name)
        : node(_node)
        , name(_name)
    {}
public:
    const Node *node;
    const std::string name;
};

struct Node
{
protected:
    Node(Node *_parent, const std::string &_name)
        : parent(_parent)
        , name(_name)
        , full_name( _parent ? _parent->full_name+"/"+_name : "/"+_name )
    {
        if(_parent){
            _parent->subNodes.push_back(this);
        }
    }

public:
    Node() = delete;
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    virtual ~Node()
    {}

    std::vector<const Node *> subNodes;
    std::vector<const LinkIn *> linksIn;
    std::vector<const LinkOut *> linksOut;

    const Node *parent = 0 ;
    const std::string name;
    const std::string full_name;

    virtual const SystemParameters * get_parameters() const
    { return parent->get_parameters(); }

    virtual FPGACoord get_fpga_coord(thread_id_t thread) const
    { return get_parameters()->get_fpga_coord(thread); }

    virtual MailboxCoord get_mailbox_coord(thread_id_t thread) const
    { return get_parameters()->get_mailbox_coord(thread); }

    virtual const Node *get_node_containing(thread_id_t dest) const
    {
        assert(parent);
        return parent->get_node_containing(dest);
    }

    virtual const LinkOut *get_link_to(thread_id_t /*dest*/) const
    {
        throw std::runtime_error("This type of node does not directly take part in routing.");
    }
};

struct LinkIn
    : Link
{
    LinkIn(Node *_node, const std::string &_name)
        : Link(_node, _name)
    {
        _node->linksIn.push_back(this);
    }

    LinkOut *source = 0;
};

struct LinkOut
    : Link
{
    LinkOut(Node *_node, const std::string &_name)
        : Link(_node, _name)
    {
        _node->linksOut.push_back(this);
    }

    LinkIn *sink = 0;

    void connect(LinkIn *_partner)
    {
        assert(node!=0 && _partner && _partner->node!=0);
        assert(sink==0 && _partner->source==0);
        sink=_partner;
        sink->source=this;
    }
};



struct ExpanderNode
    : Node
{
    ExpanderNode(Node *_parent, const std::string name, unsigned length)
        : Node(_parent, name)
        , input{this,"in"}
    {
        outputs.reserve(length); // Must make sure addresses are stable in constructor
        for(unsigned i=0; i<length; i++){
            outputs.emplace_back(this, "out["+std::to_string(i)+"]");
        }
    }

    LinkIn input;
    std::vector<LinkOut> outputs;

    const LinkOut *get_link_to(thread_id_t /*dest*/) const override
    {
        return &outputs[0]; // Tinsel always connects to first channel, and rest to null
    }
};

struct ReducerNode
    : Node
{
    ReducerNode() = delete;
    ReducerNode(const ReducerNode &) = delete;
    ReducerNode &operator=(const ReducerNode &) = delete;

    ReducerNode(Node *parent, const std::string name, unsigned length)
        : Node(parent, name)
        , output{this, "out"}
    {
        inputs.reserve(length); // Must make sure addresses are stable in constructor
        for(unsigned i=0; i<length; i++){
            inputs.emplace_back(this, "in["+std::to_string(i)+"]");
        }
    }

    std::vector<LinkIn> inputs;
    LinkOut output;

    const LinkOut *get_link_to(thread_id_t /*dest*/) const override
    {
        return &output;
    }
};

struct NullSourceNode
    : Node
{
    NullSourceNode(Node *parent, const std::string name)
        : Node(parent, name)
        , out{this,"out"}
    {}

    LinkOut out; 
};

struct NullSinkNode
    : Node
{
    NullSinkNode(Node *parent, const std::string name)
        : Node(parent, name)
        , in{this,"in"}
    {}

    LinkIn in; 
};

struct MailboxNode
    : Node
{
    MailboxNode() = delete;
    MailboxNode(const MailboxNode &) = delete;
    MailboxNode &operator=(const MailboxNode &) = delete;

    MailboxNode(Node *parent,const std::string name,  MailboxCoord _position)
        : Node(parent,name)
        , position(_position)
        , inputs{LinkIn{this,"in[North]"},LinkIn{this,"in[South]"},LinkIn{this,"in[East]"},LinkIn{this,"in[West]"}}
        , outputs{LinkOut{this,"out[North]"},LinkOut{this,"out[South]"},LinkOut{this,"out[East]"},LinkOut{this,"out[West]"}}
    {
    }

    const MailboxCoord position;
    std::array<LinkIn,4> inputs;
    std::array<LinkOut,4> outputs;

    const Node *get_node_containing(thread_id_t dest) const override
    {
        auto p=get_mailbox_coord(dest);
        if(p==position){
            return this;
        }else{
            return parent->get_node_containing(dest);
        }
    }

    const LinkOut *get_link_to(thread_id_t dest) const override
    {
        auto dir=direction_to(position, get_mailbox_coord(dest));
        if(dir==Here){
            return nullptr;
        }
        return &outputs[dir];
    }
};

struct FPGANode
    : Node
{
    FPGANode() = delete;
    FPGANode(const FPGANode &) = delete;
    FPGANode &operator=(const FPGANode &) = delete;

    FPGANode(Node *parent, const std::string name, FPGACoord _position)
        : Node(parent, name)
        , meshLengthX(parent->get_parameters()->MailboxMeshXLen)
        , meshLengthY(parent->get_parameters()->MailboxMeshYLen)
        , position(_position)
        , mailboxOrigin{ _position.x*meshLengthX, _position.x*meshLengthY }
        , expanders{{ // Expanders in North, South, East, West order
          { this, "Expander[North]", meshLengthX }, { this, "Expander[South]", meshLengthX },
          { this, "Expander[East]", meshLengthY },  { this, "Expander[West]", meshLengthY }   
        }}
        , reducers{{ // Reducers in North, South, East, West order
          { this, "Reducer[North]", meshLengthX }, { this, "Reducer[South]", meshLengthX },
          { this, "Reducer[East]", meshLengthY },  { this, "Reducer[West]", meshLengthY }   
        }}
    {
        mailboxes.resize(meshLengthX);
        for(unsigned x=0; x<meshLengthX; x++){
            mailboxes[x].resize(meshLengthY);
            for(unsigned y=0; y<meshLengthY; y++){
                std::stringstream tmp;
                tmp<<"mbox["<<x<<"]["<<y<<"]";
                auto mbox=std::make_unique<MailboxNode>(this, tmp.str(), MailboxCoord{
                    mailboxOrigin.x+x, mailboxOrigin.y+y
                });
                mailboxes[x][y]=std::move(mbox);
            }
        }


        for(unsigned x=0; x<meshLengthX; x++){
            for(unsigned y=0; y<meshLengthY; y++){
                auto &mbox=*mailboxes[x][y];

                bool northEdge = y==meshLengthY-1;
                bool southEdge = y==0;
                bool eastEdge = x==meshLengthX-1;
                bool westEdge = x==0;

                mbox.outputs[North].connect( northEdge ?  &reducers[North].inputs[x] : &mailboxes[x][y+1]->inputs[South] );
                mbox.outputs[South].connect( southEdge ?  &reducers[South].inputs[x] : &mailboxes[x][y-1]->inputs[North] );
                mbox.outputs[East].connect(  eastEdge  ?  &reducers[East].inputs[y] :  &mailboxes[x+1][y]->inputs[West] );
                mbox.outputs[West].connect(  westEdge  ?  &reducers[West].inputs[y] :  &mailboxes[x-1][y]->inputs[East] );
                
                if(northEdge){ expanders[North].outputs[x].connect( &mbox.inputs[North] ); }
                if(southEdge){ expanders[South].outputs[x].connect( &mbox.inputs[South] ); }
                if(eastEdge){  expanders[East].outputs[y].connect( &mbox.inputs[East] ); }
                if(westEdge){  expanders[West].outputs[y].connect( &mbox.inputs[West] ); }
            }
        }
    }

    const unsigned meshLengthX;
    const unsigned meshLengthY;
    const FPGACoord position;
    const MailboxCoord mailboxOrigin;

    std::array<ExpanderNode,4> expanders;
    std::array<ReducerNode,4> reducers;

    std::vector<std::vector<std::unique_ptr<MailboxNode>>> mailboxes;

    const MailboxNode *get_mailbox(int local_x, int local_y) const
    {
        return mailboxes.at(local_x).at(local_y).get();
    }

    const Node *get_node_containing(thread_id_t dest) const override
    {
        auto pf=get_fpga_coord(dest);
        if(pf==position){
            auto pm=get_mailbox_coord(dest);
            return get_mailbox(pm.x-mailboxOrigin.x, pm.y-mailboxOrigin.y);
        }else{
            return parent->get_node_containing(dest);
        }
    }
};

struct SystemNode
    : Node
{
    SystemNode(const SystemParameters *_parameters)
        : Node(nullptr, "sys")
        , parameters(_parameters)
        , fpgaCountX(_parameters->FPGAMeshXLen)
        , fpgaCountY(_parameters->FPGAMeshYLen)
        , mailboxMeshXLen(_parameters->MailboxMeshXLen)
        , mailboxMeshYLen(_parameters->MailboxMeshYLen)
    {
        fpgas.resize(fpgaCountX);
        for(unsigned x=0; x<fpgaCountX; x++){
            fpgas[x].resize(fpgaCountY);
            for(unsigned y=0; y<fpgaCountY; y++){
                std::stringstream tmp;
                tmp<<"fpga["<<x<<"]["<<y<<"]";
                fpgas.at(x).at(y)=std::make_unique<FPGANode>(this, tmp.str(), FPGACoord{x,y});
            }
        }

        for(unsigned i=0; i<fpgaCountX; i++){
            null_sinks[North].push_back(std::make_unique<NullSinkNode>(this, "null_sink[North]["+std::to_string(i)+"]"));
            null_sinks[South].push_back(std::make_unique<NullSinkNode>(this, "null_sink[South]["+std::to_string(i)+"]"));
            null_sources[North].push_back(std::make_unique<NullSourceNode>(this, "null_source[North]["+std::to_string(i)+"]"));
            null_sources[South].push_back(std::make_unique<NullSourceNode>(this, "null_source[South]["+std::to_string(i)+"]"));
        }

        for(unsigned i=0; i<fpgaCountY; i++){
            null_sinks[East].push_back(std::make_unique<NullSinkNode>(this, "null_sink[East]["+std::to_string(i)+"]"));
            null_sinks[West].push_back(std::make_unique<NullSinkNode>(this, "null_sink[West]["+std::to_string(i)+"]"));
            null_sources[East].push_back(std::make_unique<NullSourceNode>(this, "null_source[East]["+std::to_string(i)+"]"));
            null_sources[West].push_back(std::make_unique<NullSourceNode>(this, "null_source[West]["+std::to_string(i)+"]"));
        }

        for(unsigned x=0; x<fpgaCountX; x++){
            for(unsigned y=0; y<fpgaCountY; y++){
                bool northEdge = y==fpgaCountY-1;
                bool southEdge = y==0;
                bool eastEdge = x==fpgaCountX-1;
                bool westEdge = x==0;
                
                auto &fpga=*fpgas[x][y];

                fpga.reducers[North].output.connect( northEdge ? &null_sinks[North][x]->in  : &fpgas[x][y+1]->expanders[South].input );
                fpga.reducers[South].output.connect( southEdge ? &null_sinks[South][x]->in  : &fpgas[x][y-1]->expanders[North].input );
                fpga.reducers[East].output.connect( eastEdge   ? &null_sinks[East][y]->in   : &fpgas[x+1][y]->expanders[West].input );
                fpga.reducers[West].output.connect( westEdge   ? &null_sinks[West][y]->in   : &fpgas[x-1][y]->expanders[East].input );
            
                if(northEdge){ null_sources[North][x]->out.connect( &fpga.expanders[North].input); }
                if(southEdge){ null_sources[South][x]->out.connect( &fpga.expanders[South].input); }
                if(eastEdge){  null_sources[East][y]->out.connect( &fpga.expanders[East].input); }
                if(westEdge){  null_sources[West][y]->out.connect( &fpga.expanders[West].input); }
            }
        }
    }

    const SystemParameters * const parameters;

    const unsigned fpgaCountX, fpgaCountY;
    const unsigned mailboxMeshXLen, mailboxMeshYLen;

    std::vector<std::vector<std::unique_ptr<FPGANode>>> fpgas;
    std::array<std::vector<std::unique_ptr<NullSinkNode>>,4> null_sinks;
    std::array<std::vector<std::unique_ptr<NullSourceNode>>,4> null_sources;

    const SystemParameters * get_parameters() const override
    {
        return parameters;    
    }

    const FPGANode *get_fpga(int local_x, int local_y) const
    {
        return fpgas.at(local_x).at(local_y).get();
    }

    const Node *get_node_containing(thread_id_t dest) const override
    {
        auto pf=get_fpga_coord(dest);
        return get_fpga(pf.x, pf.y)->get_node_containing(dest);
    }
};

void validate_node_heirarchy(const Node *root)
{
    auto require=[](const Node *node, bool cond, const std::string &msg)
    {
        if(!cond){
            throw std::runtime_error("Invalid node heirarchy on node " + node->full_name + " : "+msg);
        }
    };

    // First build the set of reachable nodes based on subNode relationshop

    std::unordered_set<const Node *> nodes;

    std::function<void(const Node *)> collect_nodes_in_heirarchy = [&](const Node *node)
    {
        if(nodes.find(node)==nodes.end()){    
            nodes.insert(node);
            for(auto n : node->subNodes){
                require(n, n->parent==node, "Invalid parent/child relationship");
                collect_nodes_in_heirarchy(n);
            }
        }
    };
    collect_nodes_in_heirarchy(root);

    // Then walk the links and check that all links are connected, and all nodes are in reachable nodes
    for(auto n : nodes){
        for(auto in : n->linksIn){
            require(n, in->node==n, "LinkIn does not point to parent.");
            require(n, in->source!=nullptr, "LinkIn is not connected.");
            require(n, in->source->sink==in, "LinkIn source is not reflected.");
            require(n, nodes.find(in->source->node)!=nodes.end(), "LinkIn points at a non-reachable node.");
            const auto &linksOut=in->source->node->linksOut;
            require(n, std::find(linksOut.begin(),linksOut.end(),in->source)!=linksOut.end(),
                "LinkIn points to LinkOut which is not known by partner node ");
        }

        for(auto out : n->linksOut){
            require(n, out->node==n, "LinkOut does not point to parent.");
            require(n, out->sink!=nullptr, "LinkOut is not connected.");
            require(n, out->sink->source==out, "LinkOut sink is not reflected.");
            require(n, nodes.find(out->sink->node)!=nodes.end(), "LinkIn points at a non-reachable node.");
            const auto &linksIn=out->sink->node->linksIn;
            require(n, std::find(linksIn.begin(),linksIn.end(),out->sink)!=linksIn.end(),
                "LinkOut points to LinkIn which is not known by partner node ");
            
        }
    }

}



void print_node_heirarchy(std::ostream &dst, std::string prefix, const Node *node)
{
    dst<<prefix<<node->name<<"\n";
    for(auto n : node->subNodes){
        print_node_heirarchy(dst, prefix+"  ", n);
    }
}

void print_node_heirarchy_as_dot(std::ostream &dst, const Node *root)
{
    std::unordered_set<const Node *> visited;

    auto qn=[](const Node *n)
    {
        return "\""+n->full_name+"\"";
    };

    auto ql=[](const Link *l)
    {
        return "\""+l->node->full_name+":"+l->name+"\"";
    };

    std::function<void(const Node *)> visit = [&](const Node *node)
    {
        if(visited.find(node)!=visited.end()){    
            return;
        }

        visited.insert(node);

        if(node->subNodes.size() >0 && (node->linksIn.size()>0 || node->linksOut.size()>0)){
            throw std::runtime_error("This visualiser can't handle containers with their own links.");
        }

        if(node->subNodes.size()>0){
            dst<<"  subgraph \"cluster_"<<node->full_name<<"\"{\n";
            for(auto n : node->subNodes){
                visit(n);
            }
            dst<<"}\n";
        }else{
            dst<<"  "<<qn(node)<<"; \n";

            for(auto li : node->linksIn){
                dst<<"  "<<ql(li)<<";\n";
                dst<<"  "<<ql(li)<<" -> "<<qn(node)<<";\n";
            }

            for(auto lo : node->linksOut){
                dst<<"  "<<ql(lo)<<";\n";
                dst<<"  "<<qn(node)<<" -> "<<ql(lo)<<";\n";
                dst<<"  "<<ql(lo)<<" -> "<<ql(lo->sink)<<";\n";
            }
        }

    };

    dst<<"digraph G{\n";
    dst<<"  overlap = false;\n";
    visit(root);
    dst<<"}";
}

#endif
