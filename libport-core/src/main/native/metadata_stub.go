package main

import (
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"

	myhttp "github.com/jerryz920/utils/goutils/http"
	log "github.com/sirupsen/logrus"
)

type Principal struct {
	ImageID    string
	Properties map[string]string
	IP         string
	PortMin    int
	PortMax    int
}

type Image struct {
	Properties map[string]string
}

type Object struct {
	Acls []string
}

type TestStore struct {
	Principals map[string]Principal
	Images     map[string]Image
	Objects    map[string]Object
}

var (
	store = TestStore{
		Principals: make(map[string]Principal),
		Images:     make(map[string]Image),
		Objects:    make(map[string]Object),
	}
)

type MetadataRequest struct {
	Principal   string
	OtherValues []string
}

func ReadRequest(r *http.Request) (*MetadataRequest, int) {
	d := json.NewDecoder(r.Body)
	mr := MetadataRequest{}
	if err := d.Decode(&mr); err != nil {
		log.Errorf("error decoding the body %v\n", err)
		return nil, http.StatusBadRequest
	} else {
		return &mr, http.StatusOK
	}
}

var (
	ipMatch *regexp.Regexp = regexp.MustCompile(`(\d+\.\d+\.\d+\.\d+):(\d+)-(\d+)`)
)

func SetCommonHeader(w http.ResponseWriter) {
	w.Header().Set("Content-Type", "application/json")
}
func LoggedWriteHeader(w http.ResponseWriter, v int) {
	log.Info("---Response: status ", v)
	w.WriteHeader(v)
}

func LoggedWrite(w http.ResponseWriter, b []byte) {
	log.Infof("body: %s\n", string(b))
	w.Write(b)
}

func ParseIP(msg string) (string, int, int, int) {
	if matches := ipMatch.FindStringSubmatch(msg); len(matches) != 4 {
		log.Errorf("not valid principal ip-port range: %s", msg)
		return "", 0, 0, http.StatusBadRequest
	} else {
		var p1, p2 int64
		var err error
		if p1, err = strconv.ParseInt(matches[2], 10, 32); err != nil {
			log.Errorf("error parsing port min: %v", err)
			return "", 0, 0, http.StatusBadRequest
		}
		if p2, err = strconv.ParseInt(matches[3], 10, 32); err != nil {
			log.Errorf("error parsing port max: %v", err)
			return "", 0, 0, http.StatusBadRequest
		}
		return matches[1], int(p1), int(p2), http.StatusOK
	}
}

func GetPrincipal(ip string) (*Principal, int) {
	parts := strings.Split(ip, ":")
	if len(parts) != 2 || net.ParseIP(parts[0]) == nil {
		log.Errorf("error parsing IP: %s\n", ip)
		return nil, http.StatusBadRequest
	}
	var found *Principal = nil
	port, err := strconv.ParseInt(parts[1], 10, 32)
	log.Infof("Looking for principal of %s-%d\n\n, current dicts: %v", parts[0], port,
		store.Principals)
	if err != nil {
		log.Errorf("error parsing port: %v\n", err)
		return nil, http.StatusBadRequest
	}
	for _, p := range store.Principals {
		// Find the deepest nested principal
		tmp := p
		if found == nil {
			if parts[0] == p.IP && int(port) >= p.PortMin && int(port) <= p.PortMax {
				found = &tmp
			}
		} else {
			if parts[0] == p.IP && found.PortMin <= p.PortMin && found.PortMax >= p.PortMin {
				found = &tmp
			}
		}
	}
	if found != nil {
		return found, http.StatusOK
	} else {
		return nil, http.StatusNotFound
	}

}

func postInstanceSet(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	if _, ok := store.Principals[m.OtherValues[0]]; ok {
		LoggedWriteHeader(w, http.StatusConflict)
		log.Infof("request existed: %v", m.OtherValues)
	} else if len(m.OtherValues) != 5 {
		LoggedWriteHeader(w, http.StatusBadRequest)
		log.Infof("bad request values: %v", m.OtherValues)
	} else {
		ip, p1, p2, status := ParseIP(m.OtherValues[3])
		if status != http.StatusOK || p1 > 65535 || p2 > 65535 || p1 > p2 || net.ParseIP(ip) == nil {
			LoggedWriteHeader(w, status)
			log.Infof("bad request values: %v", m.OtherValues)
			return
		}
		store.Principals[m.OtherValues[0]] = Principal{
			ImageID: m.OtherValues[1],
			Properties: map[string]string{
				"config": m.OtherValues[4],
			},
			IP:      ip,
			PortMin: p1,
			PortMax: p2,
		}
		LoggedWrite(w, []byte(fmt.Sprintf(`{"message":"['%s']"}`, m.OtherValues[0])))
	}
}

func updateSubjectSet(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	if _, ok := store.Principals[m.OtherValues[0]]; !ok {
		LoggedWriteHeader(w, http.StatusNotFound)
	} else {
		LoggedWriteHeader(w, http.StatusOK)
	}
}

func postAttesterImage(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	if _, ok := store.Images[m.OtherValues[0]]; ok {
		LoggedWriteHeader(w, http.StatusConflict)
	} else {
		store.Images[m.OtherValues[0]] = Image{
			Properties: make(map[string]string),
		}
		LoggedWriteHeader(w, http.StatusOK)
	}
}

func postImageProperty(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	if i, ok := store.Images[m.OtherValues[0]]; !ok {
		LoggedWriteHeader(w, http.StatusNotFound)
	} else {
		i.Properties[m.OtherValues[1]] = "set"
		log.Infof("posting property %v for %s, image store: %v",
			m.OtherValues[1], m.OtherValues[0], store.Images)

		LoggedWriteHeader(w, http.StatusOK)
	}
}

func postObjectAcl(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	var o Object
	var ok bool
	if o, ok = store.Objects[m.OtherValues[0]]; !ok {
		o = Object{[]string{m.OtherValues[1]}}
		store.Objects[m.OtherValues[0]] = o
	} else {
		o.Acls = append(o.Acls, m.OtherValues[1])
	}
}

func attestAppProperty(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	var i Image
	var p *Principal
	if p, status = GetPrincipal(m.OtherValues[0]); status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	} else {
		var ok bool
		if i, ok = store.Images[p.ImageID]; !ok {
			LoggedWriteHeader(w, http.StatusNotFound)
			LoggedWrite(w, []byte(`{"message": "image not found"}`))
			return
		}
		if _, ok = i.Properties[m.OtherValues[1]]; !ok {
			LoggedWriteHeader(w, http.StatusNotFound)
			LoggedWrite(w, []byte(`{"message": "object not found"}`))
			return
		} else {
			LoggedWriteHeader(w, http.StatusOK)
			LoggedWrite(w, []byte(`{"message": "{ 'void':programHasProperty('','') }"}`))
		}
	}
}

func attestObjectAccess(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	var p *Principal
	var o Object
	var i Image
	if p, status = GetPrincipal(m.OtherValues[0]); status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	var ok bool
	if o, ok = store.Objects[m.OtherValues[1]]; !ok {
		LoggedWriteHeader(w, http.StatusNotFound)
		LoggedWrite(w, []byte(`{"message": "object not found"}`))
		return
	}
	if i, ok = store.Images[p.ImageID]; !ok {
		LoggedWriteHeader(w, http.StatusNotFound)
		LoggedWrite(w, []byte(`{"message": "image not found"}`))
		return
	}
	for _, wanted := range o.Acls {
		log.Infof("image info: %v", i)
		if _, ok := i.Properties[wanted]; ok {
			LoggedWrite(w, []byte(`{"message":  "{ 'void':approveAccess('','') }"}`))
			return
		}
	}
	LoggedWriteHeader(w, http.StatusForbidden)
	LoggedWrite(w, []byte(`{"message" : "access denied"}`))
}

func retractInstanceSet(w http.ResponseWriter, r *http.Request) {
	m, status := ReadRequest(r)
	SetCommonHeader(w)
	if status != http.StatusOK {
		LoggedWriteHeader(w, status)
		return
	}
	delete(store.Principals, m.OtherValues[0])
}

func main() {
	server := myhttp.NewEchoServer()
	server.AddRoute("/postInstanceSet", postInstanceSet)
	server.AddRoute("/retractInstanceSet", retractInstanceSet)
	server.AddRoute("/updateSubjectSet", updateSubjectSet)
	server.AddRoute("/postAttesterImage", postAttesterImage)
	server.AddRoute("/postImageProperty", postImageProperty)
	server.AddRoute("/postObjectAcl", postObjectAcl)
	server.AddRoute("/attestAppProperty", attestAppProperty)
	server.AddRoute("/appAccessesObject", attestObjectAccess)

	if len(os.Args) > 1 {
		server.ListenAndServe(os.Args[1])
	} else {
		server.ListenAndServe(":10011")
	}
}
