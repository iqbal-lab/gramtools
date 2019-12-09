#include "prg/coverage_graph.hpp"

/**
 * Shared_ptr in boost get destroyed when their reference_count goes to 0>
 * By default, because the graph only stores one such pointer (to the root)
 * and the graph is defined recursively from the root, boost will recursively
 * destroy the graph, which can overflow the stack.
 *
 * Destroying rightmost bubbles first avoids this.
 */
coverage_Graph::~coverage_Graph(){
    for (auto& bubble : bubble_map){
        bubble.first.get()->clear_edges();
        bubble.second.get()->clear_edges();
    }
}

coverage_Graph::coverage_Graph(PRG_String const &vec_in) {
    auto built_graph = cov_Graph_Builder(vec_in);
    root = built_graph.root;
    bubble_map = std::move(built_graph.bubble_map);
    par_map = std::move(built_graph.par_map);
    random_access = std::move(built_graph.random_access);
    target_map = std::move(built_graph.target_map);

    par_map.size() != 0 ? is_nested = true : is_nested = false;
}

bool operator==(coverage_Graph const& f, coverage_Graph const& s){
    // Test that the random_access vectors are the same, by testing each node
    node_access first;
    node_access second;
    if (f.random_access.size() != s.random_access.size()) return false;
    for (int i = 0; i < f.random_access.size(); ++i){
        first = f.random_access[i];
        second = s.random_access[i];
        bool same_node = (*(first.node) == *(second.node));
        if (!same_node) return false;
        if (first.offset != second.offset) return false;
        if (first.target != second.target) return false;
    }

    return (f.par_map == s.par_map && f.target_map == s.target_map);
}

cov_Graph_Builder::cov_Graph_Builder(PRG_String const& prg_string) {
    linear_prg = prg_string.get_PRG_string();
    random_access = access_vec(linear_prg.size(), node_access());
    end_positions = prg_string.get_end_positions();
    make_root();
    cur_Locus = std::make_pair(0, 0); // Meaning: there is currently no current Locus.

    for(uint32_t i = 0; i < linear_prg.size(); ++i) {
        process_marker(i);
        setup_random_access(i);
    }
    make_sink();
    map_targets();
}

void cov_Graph_Builder::make_root() {
    cur_pos = -1;
   root = boost::make_shared<coverage_Node>(coverage_Node(cur_pos));
   backWire = root;
   cur_pos++;
   cur_Node = boost::make_shared<coverage_Node>(coverage_Node(cur_pos));
}

void cov_Graph_Builder::make_sink() {
    auto sink = boost::make_shared<coverage_Node>(coverage_Node(cur_pos + 1));
    wire(sink);
    cur_Node = nullptr;
    backWire = nullptr;
}

void cov_Graph_Builder::process_marker(uint32_t const &pos) {
    Marker m = linear_prg[pos];
    marker_type t = find_marker_type(pos);

    switch(t){
        case marker_type::sequence:
            add_sequence(m);
            break;
        case marker_type::site_entry:
            enter_site(m);
            break;
        case marker_type::allele_end:
            end_allele(m);
            break;
        case marker_type::site_end:
            exit_site(m);
    }
}

void cov_Graph_Builder::setup_random_access(uint32_t const& pos){
    marker_type t = find_marker_type(pos);
    // Set up random access
    covG_ptr target;
    t == marker_type::sequence ? target = cur_Node : target = backWire;
    auto seq_size = target->get_sequence_size();
    if (seq_size <= 1) // Will include all site entry and exit nodes, and sequence nodes with a single character
    random_access[pos] = node_access{target, 0, VariantLocus{}};
    else random_access[pos] = node_access{target, seq_size - 1, VariantLocus{}};
};

marker_type cov_Graph_Builder::find_marker_type(uint32_t const& pos) {
    auto const& m = linear_prg[pos];
    if (m <= 4) return marker_type::sequence; // Note: the `PRG_String` constructor code must make sure that m is > 0.

    // After passing through `PRG_String` constructor code, the only time a marker is odd is when it signals a site entry.
    if (m % 2 == 1) return marker_type::site_entry;

    // Find if the allele marker, which must be in the map, is at the end position or not
    auto& end_pos = end_positions.at(m);
    assert(pos <= end_pos); // Sanity check: the allele marker must occur before, or at, the end position of the site
    if (pos < end_pos) return marker_type::allele_end;

    return marker_type::site_end;
}

void cov_Graph_Builder::add_sequence(Marker const &m) {
    std::string c = decode_dna_base(m); // Note: implicit conversion of m from uint32_t to uint8_t; this is OK because 0 < m < 5.
    cur_Node->add_sequence(c);
    cur_pos++;
}

void cov_Graph_Builder::enter_site(Marker const &m) {

    auto site_entry = boost::make_shared<coverage_Node>(coverage_Node("", cur_pos, m, 0));
    site_entry->mark_as_boundary();
    wire(site_entry);

    // Update the global pointers
    cur_Node = boost::make_shared<coverage_Node>(coverage_Node("", cur_pos, m, 1));
    backWire = site_entry;

    // Make & register a new bubble
    auto site_exit = boost::make_shared<coverage_Node>(coverage_Node("", cur_pos, m, 0));
    site_exit->mark_as_boundary();
    bubble_map.insert(std::make_pair(site_entry,site_exit));
    bubble_starts.insert(std::make_pair(m, site_entry));
    bubble_ends.insert(std::make_pair(m, site_exit));

    // Update the parent map & the current Locus
    if (cur_Locus.first != 0) {
        assert(par_map.find(m) == par_map.end()); // The marker should not be in there already
        par_map.insert(std::make_pair(m, cur_Locus));
    }
    cur_Locus = std::make_pair(m, 1);
}

void cov_Graph_Builder::end_allele(Marker const &m) {
    auto site_ID = m - 1;
    auto site_exit = reach_allele_end(m);

    auto& allele_ID = cur_Locus.second;
    // Reset node and position to the site start node
    auto site_entry = bubble_starts.at(site_ID);
    backWire = site_entry;
    cur_pos = site_entry->get_pos();

    // Update to the next allele
    allele_ID++; // increment allele ID.
    cur_Node = boost::make_shared<coverage_Node>(coverage_Node("", cur_pos, site_ID, allele_ID));
}

void cov_Graph_Builder::exit_site(Marker const &m) {
    auto site_ID = m - 1;
    auto site_exit = reach_allele_end(m);

    // Update the current Locus
    try {
        cur_Locus = par_map.at(site_ID);
    }
    // Means we were in a level 1 site; we will no longer be in a site
    catch(std::out_of_range&e){
        cur_Locus = std::make_pair(0,0);
    }

    backWire = site_exit;
    cur_pos = site_exit->get_pos(); // Take the largest allele pos as the new current pos.
    cur_Node = boost::make_shared<coverage_Node>(coverage_Node("", cur_pos, cur_Locus.first, cur_Locus.second));
}

covG_ptr cov_Graph_Builder::reach_allele_end(Marker const& m){
    // Make sure we are tracking the right site
    auto site_ID = m - 1;
    assert(cur_Locus.first == site_ID);

    auto site_exit = bubble_ends.at(site_ID);
    wire(site_exit);

    // Update the exit's pos if it is smaller than this allele's
    if (site_exit->get_pos() < cur_pos) site_exit->set_pos(cur_pos);

    return site_exit;
}


void cov_Graph_Builder::wire(covG_ptr const &target) {
    if (cur_Node->has_sequence()){
        backWire->add_edge(cur_Node);
        cur_Node->add_edge(target);
    }
    else backWire->add_edge(target);
}

void cov_Graph_Builder::map_targets(){
   marker_type prev_t = marker_type::sequence;
   Marker prev_m = 0;
   marker_type cur_t;
   Marker cur_m;
   Marker cur_allele_ID = 0;

   int pos = 0;
   while (pos < linear_prg.size()){
       cur_m = linear_prg[pos];
       cur_t = find_marker_type(pos);

       switch(cur_t){
           case marker_type::sequence:
               if (prev_t != marker_type::sequence) random_access[pos].target =
                       VariantLocus{prev_m, cur_allele_ID}; // Adds a target for the sequence character
               break;
           case marker_type::site_entry:
               cur_allele_ID = 1;
               if (prev_t != marker_type::sequence) entry_targets(prev_t, prev_m, cur_m);
               break;
           case marker_type::site_end:
               if (prev_t != marker_type::sequence)
                   // Reject empty variant sites by prohibiting prev_t to be a site entry
                   assert(prev_t != marker_type::site_entry);
                   allele_exit_targets(prev_t, prev_m, cur_m, cur_allele_ID);
               // Get the allele ID using the parental map
               if (par_map.find(cur_m - 1) != par_map.end()) cur_allele_ID = par_map.at(cur_m - 1).second;
               else cur_allele_ID = 0;
               break;
           case marker_type::allele_end:
               if (prev_t != marker_type::sequence) allele_exit_targets(prev_t, prev_m, cur_m, cur_allele_ID);
               cur_allele_ID++;
               break;
       }
       prev_m = cur_m;
       prev_t = cur_t;
       pos++;
   }
};

void cov_Graph_Builder::entry_targets(marker_type prev_t, Marker prev_m, Marker cur_m){
    Marker inserted;
    switch(prev_t){
        case marker_type::site_entry: // Double entry
        case marker_type::site_end: // End site goes straight to start site
            inserted = prev_m;
            break;
        case marker_type::allele_end: // Double entry
            inserted = prev_m - 1;
        break;
    }
    auto new_vec = std::vector<targeted_marker>();
    targeted_marker new_targeted_marker{inserted, 0};
    new_vec.emplace_back(new_targeted_marker);
    target_map.insert(std::make_pair(cur_m, new_vec));
};

void cov_Graph_Builder::allele_exit_targets(marker_type prev_t, Marker prev_m, Marker cur_m, Marker cur_allele_ID) {

    targeted_marker new_targeted_marker{prev_m, 0};
    switch(prev_t) {
        case marker_type::site_end: // Double exit
            add_exit_target(cur_m, new_targeted_marker);
            break;
        case marker_type::site_entry: // Direct deletion
        case marker_type::allele_end: // Direct deletion
            new_targeted_marker = targeted_marker{prev_m - 1, cur_allele_ID}; // Switch to targeting the site (odd) marker
            add_exit_target(cur_m, new_targeted_marker);
            break;
    }
}

void cov_Graph_Builder::add_exit_target(Marker cur_m, targeted_marker& new_t_m){
    auto new_vec = std::vector<targeted_marker>();
    new_vec.emplace_back(new_t_m);
    if (target_map.find(cur_m) != target_map.end()){
        target_map.at(cur_m).emplace_back(new_t_m);
    }
    else target_map.insert(std::make_pair(cur_m, new_vec));
}


bool operator>(const covG_ptr &lhs, const covG_ptr &rhs) {
    if (lhs->pos == rhs->pos) {
        return lhs.get() > rhs.get();
    } else return lhs->pos > rhs->pos;
}

bool operator==(coverage_Node const& f, coverage_Node const& s){
    bool same_nodes = compare_nodes(f,s); // Compare the nodes directly
    if (!same_nodes || f.next.size() != s.next.size()) return false;
    // And now compare the outgoing edges as well (but not recursively)
    for (int i = 0; i < f.next.size(); ++i){
        same_nodes = compare_nodes(*(f.next[i]), *(s.next[i]));
        if (!same_nodes) return false;
    }
    return true;
}

bool compare_nodes(coverage_Node const& f, coverage_Node const& s){
    return (
            f.sequence == s.sequence &&
            f.pos == s.pos &&
            f.site_ID == s.site_ID &&
            f.allele_ID == s.allele_ID &&
            f.coverage == s.coverage &&
            f.is_site_boundary == s.is_site_boundary
    );
}

std::ostream& operator<<(std::ostream& out, coverage_Node const& node){
   out << "Seq: " << node.sequence << std::endl;
    out << "Pos: " << node.pos << std::endl;
    out << "Site ID: " << node.site_ID << std::endl;
    out << "Allele ID: " << node.allele_ID << std::endl;
    out << "Cov: ";
    for (auto &s : node.coverage) std::cout << s << " ";
    std::cout << std::endl;
    out << "Is a site boundary: " << node.is_site_boundary << std::endl;
   return out;
}

bool operator==(targeted_marker const& f, targeted_marker const& s){
    return f.ID == s.ID && f.direct_deletion_allele == s.direct_deletion_allele;
}