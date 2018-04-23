/*
  3dfier: takes 2D GIS datasets and "3dfies" to create 3D city models.

  Copyright (C) 2015-2016  3D geoinformation research group, TU Delft

  This file is part of 3dfier.

  3dfier is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  3dfier is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with 3difer.  If not, see <http://www.gnu.org/licenses/>.

  For any information or further details about the use of 3dfier, contact
  Hugo Ledoux
  <h.ledoux@tudelft.nl>
  Faculty of Architecture & the Built Environment
  Delft University of Technology
  Julianalaan 134, Delft 2628BL, the Netherlands
*/

#include "TopoFeature.h"
#include "io.h"

int TopoFeature::_count = 0;

//-----------------------------------------------------------------------------

TopoFeature::TopoFeature(char *wkt, std::string layername, AttributeMap attributes, std::string pid) {
  _id = pid;
  _counter = _count++;
  _toplevel = true;
  _bVerticalWalls = false;
  _p2 = new Polygon2();
  bg::read_wkt(wkt, *_p2);
  bg::unique(*_p2); //-- remove duplicate vertices
  bg::correct(*_p2); //-- correct the orientation of the polygons!

  _adjFeatures = new std::vector<TopoFeature*>;
  _p2z.resize(bg::num_interior_rings(*_p2) + 1);
  _p2z[0].resize(bg::num_points(_p2->outer()));
  _lidarelevs.resize(bg::num_interior_rings(*_p2) + 1);
  _lidarelevs[0].resize(bg::num_points(_p2->outer()));
  for (int i = 0; i < bg::num_interior_rings(*_p2); i++) {
    _p2z[i + 1].resize(bg::num_points(_p2->inners()[i]));
    _lidarelevs[i + 1].resize(bg::num_points(_p2->inners()[i]));
  }
  _attributes = attributes;
  _layername = layername;
}

TopoFeature::~TopoFeature() {
  // TODO: clear memory properly
  std::clog << "I am dead now.\n";
}

Box2 TopoFeature::get_bbox2d() {
  return bg::return_envelope<Box2>(*_p2);
}

std::string TopoFeature::get_id() {
  return _id;
}

std::string  TopoFeature::get_layername() {
  return _layername;
}

bool TopoFeature::buildCDT() {
  getCDT(_p2, _p2z, _vertices, _triangles);
  return true;
}

int TopoFeature::get_counter() {
  return _counter;
}

bool TopoFeature::get_top_level() {
  return _toplevel;
}

void TopoFeature::set_top_level(bool toplevel) {
  _toplevel = toplevel;
}

Polygon2* TopoFeature::get_Polygon2() {
  return _p2;
}


void TopoFeature::get_cityjson_geom(nlohmann::json& g, std::unordered_map<std::string,unsigned long> &dPts, std::string primitive) {
  g["type"] = primitive;
  g["lod"] = 1;
  g["boundaries"];
  std::vector<std::vector<std::vector<unsigned long>>>  shelli;
  for (auto& t : _triangles) {
    unsigned long a, b, c;
    auto it = dPts.find(_vertices[t.v0].second);
    if (it == dPts.end()) {
      a = dPts.size();
      dPts[_vertices[t.v0].second] = a;
    }
    else
      a = it->second;
    it = dPts.find(_vertices[t.v1].second);
    if (it == dPts.end()) {
      b = dPts.size();
      dPts[_vertices[t.v1].second] = b;
    }
    else
      b = it->second;
    it = dPts.find(_vertices[t.v2].second);
    if (it == dPts.end()) {
      c = dPts.size();
      dPts[_vertices[t.v2].second] = c;
    }
    else
      c = it->second;
    if ((a != b) && (a != c) && (b != c))
      shelli.push_back({{a, b, c}});
  }
  for (auto& t : _triangles_vw) {
    unsigned long a, b, c;
    auto it = dPts.find(_vertices_vw[t.v0].second);
    if (it == dPts.end()) {
      a = dPts.size();
      dPts[_vertices_vw[t.v0].second] = a;
    }
    else
      a = it->second;
    it = dPts.find(_vertices_vw[t.v1].second);
    if (it == dPts.end()) {
      b = dPts.size();
      dPts[_vertices_vw[t.v1].second] = b;
    }
    else
      b = it->second;
    it = dPts.find(_vertices_vw[t.v2].second);
    if (it == dPts.end()) {
      c = dPts.size();
      dPts[_vertices_vw[t.v2].second] = c;
    }
    else
      c = it->second;
    if ((a != b) && (a != c) && (b != c)) 
      shelli.push_back({{a, b, c}});
  }
  if (primitive == "MultiSurface")
    g["boundaries"] = shelli;
  else
    g["boundaries"].push_back(shelli);
}

void TopoFeature::get_obj(std::unordered_map< std::string, unsigned long > &dPts, std::string mtl, std::string &fs) {
  fs += mtl; fs += "\n";
  for (auto& t : _triangles) {
    unsigned long a, b, c;
    auto it = dPts.find(_vertices[t.v0].second);
    if (it == dPts.end()) {
      // first get the size + 1 and then store the size in dPts due to unspecified order of execution
      a = dPts.size() + 1;
      dPts[_vertices[t.v0].second] = a;
    }
    else
      a = it->second;
    it = dPts.find(_vertices[t.v1].second);
    if (it == dPts.end()) {
      b = dPts.size() + 1;
      dPts[_vertices[t.v1].second] = b;
    }
    else
      b = it->second;
    it = dPts.find(_vertices[t.v2].second);
    if (it == dPts.end()) {
      c = dPts.size() + 1;
      dPts[_vertices[t.v2].second] = c;
    }
    else
      c = it->second;

    if ((a != b) && (a != c) && (b != c)) {
      fs += "f "; fs += std::to_string(a); fs += " "; fs += std::to_string(b); fs += " "; fs += std::to_string(c); fs += "\n";
    }
    // else
    //   std::clog << "COLLAPSED TRIANGLE REMOVED\n";
  }

  //-- vertical triangles
  if (_bVerticalWalls == true && _triangles_vw.size() > 0) {
    fs += mtl; fs += "Wall"; fs += "\n";
  }

  for (auto& t : _triangles_vw) {
    unsigned long a, b, c;
    auto it = dPts.find(_vertices_vw[t.v0].second);
    if (it == dPts.end()) {
      a = dPts.size() + 1;
      dPts[_vertices_vw[t.v0].second] = a;
    }
    else
      a = it->second;
    it = dPts.find(_vertices_vw[t.v1].second);
    if (it == dPts.end()) {
      b = dPts.size() + 1;
      dPts[_vertices_vw[t.v1].second] = b;
    }
    else
      b = it->second;
    it = dPts.find(_vertices_vw[t.v2].second);
    if (it == dPts.end()) {
      c = dPts.size() + 1;
      dPts[_vertices_vw[t.v2].second] = c;
    }
    else
      c = it->second;

    if ((a != b) && (a != c) && (b != c)) {
      fs += "f "; fs += std::to_string(a); fs += " "; fs += std::to_string(b); fs += " "; fs += std::to_string(c); fs += "\n";
    }
    // else
    //   std::clog << "COLLAPSED TRIANGLE REMOVED\n";
  }
}

AttributeMap TopoFeature::get_attributes() {
  return _attributes;
}

void TopoFeature::get_imgeo_object_info(std::ostream& of, std::string id) {
  std::string attribute;
  if (get_attribute("creationDate", attribute)) {
    of << "<imgeo:creationDate>" << attribute << "</imgeo:creationDate>";
  }
  if (get_attribute("terminationDate", attribute)) {
    of << "<imgeo:terminationDate>" << attribute << "</imgeo:terminationDate>";
  }
  if (get_attribute("lokaalid", attribute)) {
    of << "<imgeo:identificatie>";
    of << "<imgeo:NEN3610ID>";
    of << "<imgeo:namespace>NL.IMGeo</imgeo:namespace>";
    of << "<imgeo:lokaalID>" << attribute << "</imgeo:lokaalID>";
    of << "</imgeo:NEN3610ID>";
    of << "</imgeo:identificatie>";
  }
  if (get_attribute("tijdstipregistratie", attribute)) {
    of << "<imgeo:tijdstipRegistratie>" << attribute << "</imgeo:tijdstipRegistratie>";
  }
  if (get_attribute("eindregistratie", attribute)) {
    of << "<imgeo:eindRegistratie>" << attribute << "</imgeo:eindRegistratie>";
  }
  if (get_attribute("lv-publicatiedatum", attribute)) {
    of << "<imgeo:LV-publicatiedatum>" << attribute << "</imgeo:LV-publicatiedatum>";
  }
  if (get_attribute("bronhouder", attribute)) {
    of << "<imgeo:bronhouder>" << attribute << "</imgeo:bronhouder>";
  }
  if (get_attribute("inonderzoek", attribute)) {
    of << "<imgeo:inOnderzoek>" << attribute << "</imgeo:inOnderzoek>";
  }
  if (get_attribute("relatievehoogteligging", attribute)) {
    of << "<imgeo:relatieveHoogteligging>" << attribute << "</imgeo:relatieveHoogteligging>";
  }
  if (get_attribute("bgt-status", attribute, "bestaand")) {
    of << "<imgeo:bgt-status codeSpace=\"http://www.geostandaarden.nl/imgeo/def/2.1#Status\">" << attribute << "</imgeo:bgt-status>";
  }
  if (get_attribute("plus-status", attribute)) {
    of << "<imgeo:plus-status>" << attribute << "</imgeo:plus-status>";
  }
}

void TopoFeature::get_cityjson_attributes(nlohmann::json& f, AttributeMap attributes) {
  for (auto& attribute : attributes) {
    // add attributes except gml_id
    if (attribute.first.compare("gml_id") != 0) 
      f["attributes"][std::get<0>(attribute)] = attribute.second.second;
  }
}


void TopoFeature::get_citygml_attributes(std::ostream& of, AttributeMap attributes) {
  for (auto& attribute : attributes) {
    // add attributes except gml_id
    if (attribute.first.compare("gml_id") != 0) {
      std::string type;
      switch (attribute.second.first) {
      case OFTInteger:
        type = "int";
      case OFTReal:
        type = "double";
      case OFTDate:
        type = "date";
      default:
        type = "string";
      }
      of << "<gen:" + type + "Attribute name=\"" + std::get<0>(attribute) + "\">";
      of << "<gen:value>" + attribute.second.second + "</gen:value>";
      of << "</gen:" + type << "Attribute>";
    }
  }
}

bool TopoFeature::get_multipolygon_features(OGRLayer* layer, std::string className, bool writeAttributes, AttributeMap extraAttributes, bool writeHeights, int height_base, int height) {
  OGRFeatureDefn *featureDefn = layer->GetLayerDefn();
  OGRFeature *feature = OGRFeature::CreateFeature(featureDefn);
  OGRMultiPolygon multipolygon = OGRMultiPolygon();
  Point3 p;

  //-- add all triangles to the layer
  for (auto& t : _triangles) {
    OGRPolygon polygon = OGRPolygon();
    OGRLinearRing ring = OGRLinearRing();

    p = _vertices[t.v0].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
    p = _vertices[t.v1].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
    p = _vertices[t.v2].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());

    ring.closeRings();
    polygon.addRing(&ring);
    multipolygon.addGeometry(&polygon);
  }

  //-- add all vertical wall triangles to the layer
  for (auto& t : _triangles_vw) {
    OGRPolygon polygon = OGRPolygon();
    OGRLinearRing ring = OGRLinearRing();

    p = _vertices_vw[t.v0].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
    p = _vertices_vw[t.v1].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());
    p = _vertices_vw[t.v2].first;
    ring.addPoint(p.get<0>(), p.get<1>(), p.get<2>());

    ring.closeRings();
    polygon.addRing(&ring);
    multipolygon.addGeometry(&polygon);
  }

  feature->SetGeometry(&multipolygon);
  feature->SetField("3dfier_Id", this->get_id().c_str());
  feature->SetField("3dfier_Class", className.c_str());
  if (writeHeights) {
    feature->SetField("BaseHeight", z_to_float(height_base));
    feature->SetField("RoofHeight", z_to_float(height));
  }
  if (writeAttributes) {
    for (auto attr : _attributes) {
      if (!(attr.second.first == OFTDateTime && attr.second.second == "0000/00/00 00:00:00")) {
        feature->SetField(attr.first.c_str(), attr.second.second.c_str());
      }
    }
    
    for (auto attr : extraAttributes) {
      feature->SetField(attr.first.c_str(), attr.second.second.c_str());
    }
  }
  if (layer->CreateFeature(feature) != OGRERR_NONE) {
    std::cerr << "Failed to create feature " << this->get_id() << " in shapefile.\n";
    return false;
  }
  OGRFeature::DestroyFeature(feature);
  return true;
}

void TopoFeature::fix_bowtie() {
  //-- gather all rings
  std::vector<Ring2> rings = get_rings(_p2);

  //-- process each vertex of the polygon separately
  std::vector<int> anc, bnc;
  Point2 a, b;
  TopoFeature* fadj;
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      //-- Point a
      a = ring[ai];
      //-- find Point b
      int bi;
      if (ai == (ring.size() - 1)) {
        b = ring.front();
        bi = 0;
      }
      else {
        b = ring[ai + 1];
        bi = ai + 1;
      }
      //-- find the adjacent polygon to segment ab (fadj)
      fadj = nullptr;
      int adj_a_ringi = 0;
      int adj_a_pi = 0;
      int adj_b_ringi = 0;
      int adj_b_pi = 0;
      for (auto& adj : *(_adjFeatures)) {
        if (adj->has_segment(b, a, adj_b_ringi, adj_b_pi, adj_a_ringi, adj_a_pi) == true) {
          // if (adj->has_segment(b, a) == true) {
          fadj = adj;
          break;
        }
      }
      if (fadj == nullptr)
        continue;
      //-- check height differences: f > fadj for *both* Points a and b
      int az = this->get_vertex_elevation(ringi, ai);
      int bz = this->get_vertex_elevation(ringi, bi);
      int fadj_az = fadj->get_vertex_elevation(adj_a_ringi, adj_a_pi);
      int fadj_bz = fadj->get_vertex_elevation(adj_b_ringi, adj_b_pi);

      //-- Fix bow-ties
      if (((az > fadj_az) && (bz < fadj_bz)) || ((az < fadj_az) && (bz > fadj_bz))) {
        //std::clog << "BOWTIE:" << this->get_id() << " & " << fadj->get_id() << std::endl;
        //std::clog << this->get_class() << " & " << fadj->get_class() << std::endl;
        if (this->is_hard() && fadj->is_hard() == false) {
          //- this is hard, snap the smallest height of the soft feature to this
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            fadj->set_vertex_elevation(adj_a_ringi, adj_a_pi, az);
          }
          else {
            fadj->set_vertex_elevation(adj_b_ringi, adj_b_pi, bz);
          }
        }
        else if (this->is_hard() == false && fadj->is_hard()) {
          //- this is soft, snap the smallest height to the hard feature
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            this->set_vertex_elevation(ringi, ai, fadj_az);
          }
          else {
            this->set_vertex_elevation(ringi, bi, fadj_bz);
          }
        }
        else {
          if (abs(az - fadj_az) < abs(bz - fadj_bz)) {
            //- snap a to lowest
            if (az < fadj_az) {
              fadj->set_vertex_elevation(adj_a_ringi, adj_a_pi, az);
            }
            else
            {
              this->set_vertex_elevation(ringi, ai, fadj_az);
            }
          }
          else {
            //- snap b to lowest
            if (bz < fadj_bz) {
              fadj->set_vertex_elevation(adj_b_ringi, adj_b_pi, bz);
            }
            else {
              this->set_vertex_elevation(ringi, bi, fadj_bz);
            }
          }
        }
      }
    }
  }
}

void TopoFeature::construct_vertical_walls(NodeColumn& nc, int baseheight) {
  if (this->has_vertical_walls() == false)
    return;

  //-- gather all rings
  std::vector<Ring2> rings = get_rings(_p2);

  //-- process each vertex of the polygon separately
  std::vector<int> anc, bnc;
  std::unordered_map<std::string, std::vector<int>>::const_iterator ncit;
  Point2 a, b;
  TopoFeature* fadj;
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      //-- Point a
      a = ring[ai];
      //-- find Point b
      int bi;
      if (ai == (ring.size() - 1)) {
        b = ring.front();
        bi = 0;
      }
      else {
        b = ring[ai + 1];
        bi = ai + 1;
      }
      //-- check if there's a nc for either
      ncit = nc.find(gen_key_bucket(&a));
      if (ncit != nc.end())
        anc = ncit->second;
      ncit = nc.find(gen_key_bucket(&b));
      if (ncit != nc.end())
        bnc = ncit->second;

      if ((anc.empty() == true) && (bnc.empty() == true))
        continue;

      //-- find the adjacent polygon to segment ab (fadj)
      fadj = nullptr;
      int adj_a_ringi = 0;
      int adj_a_pi = 0;
      int adj_b_ringi = 0;
      int adj_b_pi = 0;
      for (auto& adj : *(_adjFeatures)) {
        if (adj->has_segment(b, a, adj_b_ringi, adj_b_pi, adj_a_ringi, adj_a_pi) == true) {
          fadj = adj;
          break;
        }
      }
      if (fadj == nullptr && this->get_class() != BUILDING) {
        continue;
      }

      int az = this->get_vertex_elevation(ringi, ai);
      int bz = this->get_vertex_elevation(ringi, bi);
      int fadj_az, fadj_bz;
      if(fadj == nullptr) {
        fadj_az = baseheight;
        fadj_bz = baseheight;
      }
      else {
        fadj_az = fadj->get_vertex_elevation(adj_a_ringi, adj_a_pi);
        fadj_bz = fadj->get_vertex_elevation(adj_b_ringi, adj_b_pi);
      }


      //-- Make exeption for bridges, they have vw's from bottom up, swap . Also skip if adjacent feature is bridge, vw is then created by bridge
      if (this->get_class() == BRIDGE) {
        //-- find the height of the vertex in the node column
        std::vector<int>::const_iterator sait, eait, sbit, ebit;
        sait = std::find(anc.begin(), anc.end(), az);
        eait = std::find(anc.begin(), anc.end(), fadj_az);
        sbit = std::find(bnc.begin(), bnc.end(), bz);
        ebit = std::find(bnc.begin(), bnc.end(), fadj_bz);

        //-- iterate to triangulate
        while ((sbit != ebit) && (sbit != bnc.end()) && ((sbit + 1) != bnc.end())) {
          Point3 p;
          if (anc.size() == 0 || sait == anc.end()) {
            p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(az));
            _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          }
          else {
            p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
            _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          }
          p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          sbit++;
          p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          Triangle t;
          int size = int(_vertices_vw.size());
          t.v0 = size - 1;
          t.v1 = size - 3;
          t.v2 = size - 2;
          _triangles_vw.push_back(t);
        }
        while (sait != eait && sait != anc.end() && (sait + 1) != anc.end()) {
          Point3 p;
          if (bnc.size() == 0 || ebit == bnc.end()) {
            p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(bz));
            _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          }
          else {
            p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*ebit));
            _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          }
          p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          sait++;
          p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
          Triangle t;
          int size = int(_vertices_vw.size());
          t.v0 = size - 1;
          t.v1 = size - 2;
          t.v2 = size - 3;
          _triangles_vw.push_back(t);
        }
      }
      if (fadj != nullptr && fadj->get_class() == BRIDGE) {
        continue;
      }
      //-- check height differences: f > fadj for *both* Points a and b. 
      if (az < fadj_az || bz < fadj_bz) {
        continue;
      }
      if (az == fadj_az && bz == fadj_bz) {
        continue;
      }

      //std::clog << "az: " << az << std::endl;
      //std::clog << "bz: " << bz << std::endl;
      //std::clog << "fadj_az: " << fadj_az << std::endl;
      //std::clog << "fadj_bz: " << fadj_bz << std::endl;

      //-- find the height of the vertex in the node column
      std::vector<int>::const_iterator sait, eait, sbit, ebit;
      sait = std::find(anc.begin(), anc.end(), fadj_az);
      eait = std::find(anc.begin(), anc.end(), az);
      sbit = std::find(bnc.begin(), bnc.end(), fadj_bz);
      ebit = std::find(bnc.begin(), bnc.end(), bz);

      //-- iterate to triangulate
      while ((sbit != ebit) && (sbit != bnc.end()) && ((sbit + 1) != bnc.end())) {
        Point3 p;
        if (anc.size() == 0 || sait == anc.end()) {
          p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(az));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        }
        else {
          p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        }
        p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit));
        _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        sbit++;
        p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*sbit));
        _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        Triangle t;
        int size = int(_vertices_vw.size());
        t.v0 = size - 2;
        t.v1 = size - 3;
        t.v2 = size - 1;
        _triangles_vw.push_back(t);
      }
      while (sait != eait && sait != anc.end() && (sait + 1) != anc.end()) {
        Point3 p;
        if (bnc.size() == 0 || ebit == bnc.end()) {
          p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(bz));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        }
        else {
          p = Point3(bg::get<0>(b), bg::get<1>(b), z_to_float(*ebit));
          _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        }
        p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
        _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        sait++;
        p = Point3(bg::get<0>(a), bg::get<1>(a), z_to_float(*sait));
        _vertices_vw.push_back(std::make_pair(p, gen_key_bucket(&p)));
        Triangle t;
        int size = int(_vertices_vw.size());
        t.v0 = size - 3;
        t.v1 = size - 2;
        t.v2 = size - 1;
        _triangles_vw.push_back(t);
      }
    }
  }
}

bool TopoFeature::has_segment(Point2& a, Point2& b, int& aringi, int& api, int& bringi, int& bpi) {
  double threshold = 0.001;
  std::vector<int> ringis, pis;
  Point2 tmp;
  if (this->has_point2_(a, ringis, pis) == true) {
    for (int k = 0; k < ringis.size(); k++) {
      // nextpi = pis[k];
      int nextpi;
      tmp = this->get_next_point2_in_ring(ringis[k], pis[k], nextpi);
      if (distance(b, tmp) <= threshold) {
        aringi = ringis[k];
        api = pis[k];
        bringi = ringis[k];
        bpi = nextpi;
        return true;
      }
    }
  }
  return false;
}

float TopoFeature::get_distance_to_boundaries(Point2& p) {
  //-- gather all rings
  std::vector<Ring2> rings = get_rings(_p2);
  Point2 a, b;
  Segment2 s;
  int ringi = -1;
  double dmin = 99999;
  for (auto& ring : rings) {
    ringi++;
    for (int ai = 0; ai < ring.size(); ai++) {
      a = ring[ai];
      if (ai == (ring.size() - 1))
        b = ring.front();
      else
        b = ring[ai + 1];
      //-- calculate distance
      bg::set<0, 0>(s, bg::get<0>(a));
      bg::set<0, 1>(s, bg::get<1>(a));
      bg::set<1, 0>(s, bg::get<0>(b));
      bg::set<1, 1>(s, bg::get<1>(b));
      double d = bg::distance(p, s);
      if (d < dmin)
        dmin = d;
    }
  }
  return (float)dmin;
}

bool TopoFeature::has_point2_(const Point2& p, std::vector<int>& ringis, std::vector<int>& pis) {
  double threshold = 0.001;

  //-- gather all rings
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  bool re = false;
  for (auto& ring : rings) {
    ringi++;
    for (int i = 0; i < ring.size(); i++) {
      if (distance(p, ring[i]) <= threshold) {
        ringis.push_back(ringi);
        pis.push_back(i);
        re = true;
        break;
      }
    }
  }
  return re;
}

Point2 TopoFeature::get_point2(int ringi, int pi) {
  Ring2 ring;
  if (ringi == 0)
    ring = _p2->outer();
  else
    ring = _p2->inners()[ringi - 1];
  return ring[pi];
}

Point2 TopoFeature::get_next_point2_in_ring(int ringi, int i, int& pi) {
  Ring2 ring;
  if (ringi == 0)
    ring = _p2->outer();
  else
    ring = _p2->inners()[ringi - 1];

  if (i == (ring.size() - 1)) {
    pi = 0;
    return ring.front();
  }
  else {
    pi = i + 1;
    return ring[pi];
  }
}

bool TopoFeature::has_vertical_walls() {
  return _bVerticalWalls;
}

void TopoFeature::add_vertical_wall() {
  _bVerticalWalls = true;
}

int TopoFeature::get_vertex_elevation(int ringi, int pi) {
  return _p2z[ringi][pi];
}

int TopoFeature::get_vertex_elevation(Point2& p) {
  std::vector<int> ringis, pis;
  has_point2_(p, ringis, pis);
  return _p2z[ringis[0]][pis[0]];
}

void TopoFeature::set_vertex_elevation(int ringi, int pi, int z) {
  _p2z[ringi][pi] = z;
}

//-- used to collect all points linked to the polygon
//-- later all these values are used to lift the polygon (and put values in _p2z)
bool TopoFeature::assign_elevation_to_vertex(Point2 &p, double z, float radius) {
  int zcm = int(z * 100);
  //-- collect the rings of the polygon
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    for (int i = 0; i < ring.size(); i++) {
      if (distance(p, ring[i]) <= radius)
        _lidarelevs[ringi][i].push_back(zcm);
    }
  }
  return true;
}

bool TopoFeature::within_range(Point2 &p, Polygon2 &poly, double radius) {
  //-- collect the rings of the polygon
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    //--  point is within range of the polygon rings
    for (int i = 0; i < ring.size(); i++) {
      if (distance(p, ring[i]) <= radius) {
        return true;
      }
    }
  }
  //-- point is within the polygon
  if (point_in_polygon(p, poly)) {
    return true;
  }
  return false;
}

// based on http://stackoverflow.com/questions/217578/how-can-i-determine-whether-a-2d-point-is-within-a-polygon/2922778#2922778
bool TopoFeature::point_in_polygon(const Point2 &p, const Polygon2 &poly) {
  //test outer ring
  Ring2 oring = bg::exterior_ring(poly);
  int nvert = oring.size();
  int i, j = 0;
  bool insideOuter = false;
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    double py = p.y();
    if (((oring[i].y()>py) != (oring[j].y()>py)) &&
      (p.x() < (oring[j].x() - oring[i].x()) * (py - oring[i].y()) / (oring[j].y() - oring[i].y()) + oring[i].x()))
      insideOuter = !insideOuter;
  }
  if (insideOuter) {
    //test inner rings
    auto irings = bg::interior_rings(poly);
    for (Ring2& iring : irings) {
      bool insideInner = false;
      int nvert = iring.size();
      int i, j = 0;
      for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        double py = p.y();
        if (((iring[i].y() > py) != (iring[j].y() > py)) &&
          (p.x() < (iring[j].x() - iring[i].x()) * (py - iring[i].y()) / (iring[j].y() - iring[i].y()) + iring[i].x()))
          insideInner = !insideInner;
      }
      if (insideInner) {
        return false;
      }
    }
  }
  return insideOuter;
}

void TopoFeature::get_triangle_as_gml_surfacemember(std::ostream& of, Triangle& t, bool verticalwall) {
  of << "<gml:surfaceMember>";
  of << "<gml:Polygon>";
  of << "<gml:exterior>";
  of << "<gml:LinearRing>";
  if (verticalwall == false) {
    of << "<gml:pos>" << _vertices[t.v0].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v1].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v2].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v0].second << "</gml:pos>";
  }
  else {
    of << "<gml:pos>" << _vertices_vw[t.v0].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v1].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v2].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v0].second << "</gml:pos>";
  }
  of << "</gml:LinearRing>";
  of << "</gml:exterior>";
  of << "</gml:Polygon>";
  of << "</gml:surfaceMember>";
}

void TopoFeature::get_triangle_as_gml_triangle(std::ostream& of, Triangle& t, bool verticalwall) {
  of << "<gml:Triangle>";
  of << "<gml:exterior>";
  of << "<gml:LinearRing>";
  if (verticalwall == false) {
    of << "<gml:pos>" << _vertices[t.v0].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v1].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v2].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices[t.v0].second << "</gml:pos>";
  }
  else {
    of << "<gml:pos>" << _vertices_vw[t.v0].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v1].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v2].second << "</gml:pos>";
    of << "<gml:pos>" << _vertices_vw[t.v0].second << "</gml:pos>";
  }
  of << "</gml:LinearRing>";
  of << "</gml:exterior>";
  of << "</gml:Triangle>";
}

bool TopoFeature::get_attribute(std::string attributeName, std::string &attribute, std::string defaultValue)
{
  auto it = _attributes.find(attributeName);
  if (it != _attributes.end()) {
    attribute = (*it).second.second;
    if (!attribute.empty()) {
      // attribute is empty
      return true;
    }
    if (attribute.empty() && !defaultValue.empty()) {
      // attribute is empty but default is given
      attribute = defaultValue;
      return true;
    }
  }
  // attribute does not exist
  return false;
}

void TopoFeature::lift_all_boundary_vertices_same_height(int height) {
  //-- collect the rings of the polygon
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    for (int i = 0; i < ring.size(); i++)
      _p2z[ringi][i] = height;
  }
}

void TopoFeature::add_adjacent_feature(TopoFeature* adjFeature) {
  _adjFeatures->push_back(adjFeature);
}

std::vector<TopoFeature*>* TopoFeature::get_adjacent_features() {
  return _adjFeatures;
}

void TopoFeature::lift_each_boundary_vertices(float percentile) {
  //-- 1. assign value for each vertex based on percentile
  //-- collect the rings of the polygon
  bool hasEmpty = false;
  int totalheight = 0;
  int heightcount = 0;
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    for (int i = 0; i < ring.size(); i++) {
      std::vector<int> &l = _lidarelevs[ringi][i];
      if (l.empty() == true) {
        _p2z[ringi][i] = -9999;
        hasEmpty = true;
      }
      else {
        std::nth_element(l.begin(), l.begin() + (l.size() * percentile), l.end());
        _p2z[ringi][i] = l[l.size() * percentile];
        totalheight += _p2z[ringi][i];
        heightcount++;
      }
    }
  }

  //-- 2. find average height of the polygon
  int avgheight;
  if (heightcount > 0) {
    avgheight = int(totalheight / heightcount);
  }
  else {
    avgheight = -9999;
    hasEmpty = false; // no need since all heights are -9999
    std::cout << "WARNING: Object " << _id << " doesn't have any heights." << std::endl;
  }

  if (hasEmpty) {
    //-- 3. some vertices will have no values (no lidar point within tolerance thus)
    //--    assign them the avg
    ringi = -1;
    for (auto& ring : rings) {
      ringi++;
      for (int i = 0; i < ring.size(); i++) {
        if (_p2z[ringi][i] == -9999)
          _p2z[ringi][i] = avgheight;
      }
    }
  }
}

//-------------------------------
//-------------------------------

Flat::Flat(char *wkt, std::string layername, AttributeMap attributes, std::string pid)
  : TopoFeature(wkt, layername, attributes, pid) {}

int Flat::get_number_vertices() {
  // return int(2 * _vertices.size());
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool Flat::add_elevation_point(Point2 &p, double z, float radius, int lasclass) {
  if (within_range(p, *(_p2), radius)) {
    int zcm = int(z * 100);
    //-- 1. assign to polygon since within the threshold value (buffering of polygon)
    _zvaluesinside.push_back(zcm);
  }
  return true;
}

int Flat::get_height() {
  return get_vertex_elevation(0, 0);
}

bool Flat::lift_percentile(float percentile) {
  int z = 0;
  if (_zvaluesinside.empty() == false) {
    std::nth_element(_zvaluesinside.begin(), _zvaluesinside.begin() + (_zvaluesinside.size() * percentile), _zvaluesinside.end());
    z = _zvaluesinside[_zvaluesinside.size() * percentile];
  }
  this->lift_all_boundary_vertices_same_height(z);
  _zvaluesinside.clear();
  _zvaluesinside.shrink_to_fit();
  return true;
}

//-------------------------------
//-------------------------------

Boundary3D::Boundary3D(char *wkt, std::string layername, AttributeMap attributes, std::string pid)
  : TopoFeature(wkt, layername, attributes, pid) {}

int Boundary3D::get_number_vertices() {
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool Boundary3D::add_elevation_point(Point2 &p, double z, float radius, int lasclass) {
  // no need for checking for point-in-polygon since only points in range of the vertices are added
  assign_elevation_to_vertex(p, z, radius);
  return true;
}

void Boundary3D::smooth_boundary(int passes) {
  std::vector<int> tmp;
  for (int p = 0; p < passes; p++) {
    for (auto& r : _p2z) {
      tmp.resize(r.size());
      tmp.front() = int((r[1] + r.back()) / 2);
      auto it = r.end();
      it -= 2;
      tmp.back() = int((r.front() + *it) / 2);
      for (int i = 1; i < (r.size() - 1); i++)
        tmp[i] = int((r[i - 1] + r[i + 1]) / 2);
    }
  }
}

void Boundary3D::detect_outliers(int degrees_incline) {
  //-- collect the rings of the polygon
  std::vector<Ring2> rings = get_rings(_p2);
  int ringi = -1;
  for (auto& ring : rings) {
    ringi++;
    std::vector<int> ringz = _p2z[ringi];
    float PI = 3.14159265;

    for (int i = 0; i < ring.size(); i++) {
      int i0 = i - 1;
      int i2 = i + 1;
      if (i == 0) {
        i0 = ring.size() - 1;
      }
      if (i2 == ring.size()) {
        i2 = 0;
      }
      float len1 = sqrt(pow(ring[i0].x() - ring[i].x(), 2) + pow(ring[i0].y() - ring[i].y(), 2));
      float len2 = sqrt(pow(ring[i].x() - ring[i2].x(), 2) + pow(ring[i].y() - ring[i2].y(), 2));
      float len1z = (ringz[i] - ringz[i0]) / 100.0;
      float len2z = (ringz[i2] - ringz[i]) / 100.0;
      float incline = atan2(len2z, len2) - atan2(len1z, len1);
      if (incline <= -PI) {
        incline = 2 * PI + incline;
      }
      if (incline > PI) {
        incline = incline - 2 * PI;
      }
      incline = incline * 180 / PI;

      //if (incline > 0) we have a peak down, otherwise we have a peak up
      if (abs(incline) > degrees_incline) {
        //find the outlier by sorting and comparing distance
        std::vector<int> heights = { ringz[i0], ringz[i], ringz[i2] };
        std::sort(heights.begin(), heights.end());
        int h = heights[0];
        if (abs(heights[2] - heights[1]) > abs(heights[0] - heights[1])) {
          h = heights[2];
        }
        if (ringz[i0] == h) {
          //put to height of closest vertex for now
          _p2z[0][i0] = ringz[i];
          ringz[i0] = ringz[i];
        }
        else if (ringz[i] == h) {
          _p2z[0][i] = (ringz[i0] + ringz[i2]) / 2;
          ringz[i] = (ringz[i0] + ringz[i2]) / 2;
        }
        else if (ringz[i2] == h) {
          //put to height of closest vertex for now
          _p2z[0][i2] = ringz[i];
          ringz[i2] = ringz[i];
        }
      }
    }
  }
}

// void Boundary3D::smooth_boundary(int passes) {
//   for (int p = 0; p < passes; p++) {
//     int ringi = 0;
//     Ring2 oring = bg::exterior_ring(*(_p2));
//     std::vector<int> elevs(bg::num_points(oring));
//     smooth_ring(_p2z[ringi], elevs);
//     for (int i = 0; i < oring.size(); i++) 
//       _p2z[ringi][i] = elevs[i];
//     ringi++;
//     auto irings = bg::interior_rings(*_p2);
//     for (Ring2& iring: irings) {
//       elevs.resize(bg::num_points(iring));
//       smooth_ring(_p2z[ringi], elevs);
//       for (int i = 0; i < iring.size(); i++) 
//         _p2z[ringi][i] = elevs[i];
//       ringi++;
//     }
//   }
// }

// void Boundary3D::smooth_ring(const std::vector<int> &r, std::vector<int> &elevs) {
//   elevs.front() = (bg::get<2>(r[1]) + bg::get<2>(r.back())) / 2;
//   auto it = r.end();
//   it -= 2;
//   elevs.back() = (bg::get<2>(r.front()) + bg::get<2>(*it)) / 2;
//   for (int i = 1; i < (r.size() - 1); i++) 
//     elevs[i] = (bg::get<2>(r[i - 1]) + bg::get<2>(r[i + 1])) / 2;
// }

//-------------------------------
//-------------------------------

TIN::TIN(char *wkt, std::string layername, AttributeMap attributes, std::string pid, int simplification, double simplification_tinsimp, float innerbuffer)
  : TopoFeature(wkt, layername, attributes, pid) {
  _simplification = simplification;
  _simplification_tinsimp = simplification_tinsimp;
  _innerbuffer = innerbuffer;
}

int TIN::get_number_vertices() {
  return (int(_vertices.size()) + int(_vertices_vw.size()));
}

bool TIN::add_elevation_point(Point2 &p, double z, float radius, int lasclass) {
  bool toadd = false;
  // no need for checking for point-in-polygon since only points in range of the vertices are added
  assign_elevation_to_vertex(p, z, radius);
  if (_simplification <= 1)
    toadd = true;
  else {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1, _simplification);
    if (dis(gen) == 1)
      toadd = true;
  }
  // Add the point to the lidar points if it is within the polygon and respecting the inner buffer size
  if (toadd && point_in_polygon(p, *(_p2)) && (_innerbuffer == 0.0 || (within_range(p, *(_p2), _innerbuffer) && this->get_distance_to_boundaries(p) > _innerbuffer))) {
    _lidarpts.push_back(Point3(p.x(), p.y(), z));
  }
  return toadd;
}

bool TIN::buildCDT() {
  getCDT(_p2, _p2z, _vertices, _triangles, _lidarpts, _simplification_tinsimp);
  return true;
}
