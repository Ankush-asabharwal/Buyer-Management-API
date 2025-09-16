// #include <iostream>
// #include <regex>
// #include <ctime>
// #include<string>
// #include "crow_all.h"
// using namespace std;

// struct Buyer {
//     int id;
//     string fullName;
//     string email;
//     string phone;
//     string city;
//     string propertyType;
//     string bhk;
//     string purpose;
//     int budgetMin;
//     int budgetMax;
//     string timeline;
//     string source;
//     string status;
//     int ownerId;
//     string timestamp;
//     string notes;
//     vector<string> tags;
// };

// struct BuyerHistory {
//     int id;
//     int buyerId;
//     string changedBy;
//     string changedAt;
//     crow::json::wvalue diff;
// };

// // In-memory stores
// vector<Buyer> buyers;
// vector<BuyerHistory> history;

// // Helpers
// string now() {
//     time_t t = time(nullptr);
//     char buf[64];
//     strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
//     return string(buf);
// }

// bool isValidPhone(const string& phone) {
//     if (phone.size() < 10 || phone.size() > 15) return false;
//     return all_of(phone.begin(), phone.end(), ::isdigit);
// }

// bool isValidEmail(const string& email) {
//     const regex pattern(R"((\w+)(\.{0,1})(\w*)@(\w+)(\.(\w+))+)");
//     return regex_match(email, pattern);
// }

// int main() {
//     crow::SimpleApp app;

//     CROW_ROUTE(app, "/buyers/new").methods("POST"_method)
//     ([](const crow::request& req) {
//         auto body = crow::json::load(req.body);
//         if (!body) {
//             return crow::response(400, "Invalid JSON");
//         }

//         Buyer b;
//         b.id = buyers.size() + 1;
//         b.fullName = body["fullName"].s();
//         b.email = body.has("email") ? std::string(body["email"].s()) : std::string("");
//         b.phone = body["phone"].s();
//         b.city = body["city"].s();
//         b.propertyType = body["propertyType"].s();
//         b.bhk   = body.has("bhk")   ? std::string(body["bhk"].s())   : std::string("");
//         b.purpose = body["purpose"].s();
//         b.budgetMin = body.has("budgetMin") ? body["budgetMin"].i() : 0;
//         b.budgetMax = body.has("budgetMax") ? body["budgetMax"].i() : 0;
//         b.timeline = body["timeline"].s();
//         b.source = body["source"].s();
//         b.status = "New";   // default
//         b.ownerId = 1;      // fake current user
//         b.timestamp = now();
//         b.notes = body.has("notes") ? std::string(body["notes"].s()) : std::string("");

//         if (body.has("tags")) {
//             for (size_t i = 0; i < body["tags"].size(); i++) {
//                 b.tags.push_back(body["tags"][i].s());
//             }
//         }

//         // --- Validations ---
//         if (b.fullName.size() < 2) return crow::response(400, "fullName must be at least 2 characters");
//         if (!isValidPhone(b.phone)) return crow::response(400, "phone must be numeric 10–15 digits");
//         if (!b.email.empty() && !isValidEmail(b.email)) return crow::response(400, "invalid email format");
//         if (b.budgetMax && b.budgetMin && b.budgetMax < b.budgetMin)
//             return crow::response(400, "budgetMax must be >= budgetMin");
//         if ((b.propertyType == "Apartment" || b.propertyType == "Villa") && b.bhk.empty())
//             return crow::response(400, "bhk required for Apartment/Villa");

//         // Save buyer
//         buyers.push_back(b);

//         // Save buyer_history entry
//         BuyerHistory h;
//         h.id = history.size() + 1;
//         h.buyerId = b.id;
//         h.changedBy = "user-1"; // stubbed current user
//         h.changedAt = now();
//         crow::json::wvalue diff;
//         diff["created"] = true;
//         h.diff = std::move(diff); 
//         history.push_back(h);

//         // --- Response ---
//         crow::json::wvalue res;
//         res["id"] = b.id;
//         res["message"] = "Buyer created successfully";
//         res["status"] = b.status;
//         return crow::response(201, res);
//     });

//     app.port(18080).multithreaded().run();
//     return 0;
// }



#include <iostream>
#include <fstream>
#include <map>
#include <ctime>
#include <regex>
#include <vector>
#include <libpq-fe.h>
#include "crow_all.h"

using namespace std;

// ---------------- Properties Loader ----------------
map<string,string> loadProperties(const string& filename) {
    map<string,string> props;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0]=='#') continue;
        size_t eq = line.find('=');
        if (eq!=string::npos) {
            string key=line.substr(0,eq);
            string val=line.substr(eq+1);
            props[key]=val;
        }
    }
    return props;
}

// ---------------- Helpers ----------------
string now() {
    time_t t = time(nullptr);
    char buf[64];
    strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&t));
    return string(buf);
}

bool isValidPhone(const string& phone) {
    if (phone.size()<10 || phone.size()>15) return false;
    return all_of(phone.begin(),phone.end(),::isdigit);
}

bool isValidEmail(const string& email) {
    const regex pattern(R"((\w+)(\.{0,1})(\w*)@(\w+)(\.(\w+))+)"); 
    return regex_match(email, pattern);
}

// ---------------- Main ----------------
int main() {
    // Load DB properties
    auto props = loadProperties("db.properties");
    string conninfo = "host=" + props["host"] +
                      " port=" + props["port"] +
                      " dbname=" + props["database"] +
                      " user=" + props["user"] +
                      " password=" + props["password"];

    // Connect once (reuse later)
    PGconn* conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "DB connection failed: " << PQerrorMessage(conn);
        PQfinish(conn);
        return 1;
    }

    // Ensure tables exist
    PQexec(conn,
        "CREATE EXTENSION IF NOT EXISTS pgcrypto; "
        "CREATE TABLE IF NOT EXISTS buyers ("
        " id UUID PRIMARY KEY DEFAULT gen_random_uuid(),"
        " fullName TEXT NOT NULL,"
        " email TEXT,"
        " phone TEXT NOT NULL,"
        " city TEXT,"
        " propertyType TEXT,"
        " bhk TEXT,"
        " purpose TEXT,"
        " budgetMin INT,"
        " budgetMax INT,"
        " timeline TEXT,"
        " source TEXT,"
        " status TEXT DEFAULT 'New',"
        " notes TEXT,"
        " tags JSONB,"
        " ownerId TEXT,"
        " updatedAt TIMESTAMP DEFAULT NOW()"
        ");"
        "CREATE TABLE IF NOT EXISTS buyer_history ("
        " id SERIAL PRIMARY KEY,"
        " buyerId UUID,"
        " changedBy TEXT,"
        " changedAt TIMESTAMP,"
        " diff JSONB"
        ");"
    );

    crow::SimpleApp app;

    // -------- Create Buyer --------
    CROW_ROUTE(app, "/buyers/new").methods("POST"_method)
    ([conn](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400,"Invalid JSON");

        string fullName = body["fullName"].s();
        string phone    = body["phone"].s();
        string email    = body.has("email")? string(body["email"].s()):"";
        string city     = body["city"].s();
        string propertyType = body["propertyType"].s();
        string bhk      = body.has("bhk")? string(body["bhk"].s()):"";
        string purpose  = body["purpose"].s();
        int budgetMin   = body.has("budgetMin")? body["budgetMin"].i():0;
        int budgetMax   = body.has("budgetMax")? body["budgetMax"].i():0;
        string timeline = body["timeline"].s();
        string source   = body["source"].s();
        string notes    = body.has("notes")? string(body["notes"].s()):"";
        // string tags     = body.has("tags") ? body["tags"].dump() : "[]";
        string tags = "[]";


        // ---- Validations ----
        if (fullName.size()<2) return crow::response(400,"fullName too short");
        if (!isValidPhone(phone)) return crow::response(400,"Invalid phone");
        if (!email.empty() && !isValidEmail(email)) return crow::response(400,"Invalid email");
        if (budgetMax && budgetMin && budgetMax<budgetMin) 
            return crow::response(400,"budgetMax < budgetMin");
        if ((propertyType=="Apartment"||propertyType=="Villa") && bhk.empty())
            return crow::response(400,"bhk required");

        // ---- Insert Buyer ----
        const char* params[15];
        string budgetMinStr = to_string(budgetMin);
        string budgetMaxStr = to_string(budgetMax);

        params[0]=fullName.c_str();
        params[1]=email.c_str();
        params[2]=phone.c_str();
        params[3]=city.c_str();
        params[4]=propertyType.c_str();
        params[5]=bhk.c_str();
        params[6]=purpose.c_str();
        params[7]=budgetMinStr.c_str();
        params[8]=budgetMaxStr.c_str();
        params[9]=timeline.c_str();
        params[10]=source.c_str();
        params[11]="New";
        params[12]=notes.c_str();
        params[13]=tags.c_str();
        params[14]="user-1";

        PGresult* res = PQexecParams(conn,
            "INSERT INTO buyers (fullName,email,phone,city,propertyType,bhk,purpose,"
            "budgetMin,budgetMax,timeline,source,status,notes,tags,ownerId) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15) "
            "RETURNING id",
            15, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res)!=PGRES_TUPLES_OK) {
            string err = PQerrorMessage(conn);
            PQclear(res);
            return crow::response(500,"Insert failed: "+err);
        }

        string buyerId = PQgetvalue(res,0,0);
        PQclear(res);

        // ---- Insert Buyer History ----
        const char* histParams[4];
        histParams[0]=buyerId.c_str();
        histParams[1]="user-1";
        string changedAt=now();
        histParams[2]=changedAt.c_str();
        string diff="{\"created\":true}";
        histParams[3]=diff.c_str();

        PGresult* res2 = PQexecParams(conn,
            "INSERT INTO buyer_history (buyerId,changedBy,changedAt,diff) "
            "VALUES ($1,$2,$3,$4)",
            4,nullptr,histParams,nullptr,nullptr,0);
        PQclear(res2);

        crow::json::wvalue out;
        out["id"]=buyerId;
        out["message"]="Buyer created";
        out["status"]="New";
        return crow::response(201,out);
    });

        // -------- List Buyers --------
    CROW_ROUTE(app, "/buyers").methods("GET"_method)
    ([conn](const crow::request& req){
        int page = req.url_params.get("page")? stoi(req.url_params.get("page")):1;
        int pageSize = 10;
        int offset = (page-1)*pageSize;

        string limitStr=to_string(pageSize);
        string offsetStr=to_string(offset);
        const char* params[2]={limitStr.c_str(),offsetStr.c_str()};

        PGresult* res = PQexecParams(conn,
            "SELECT id,fullName,phone,city,propertyType,budgetMin,budgetMax,timeline,status,updatedAt "
            "FROM buyers ORDER BY updatedAt DESC LIMIT $1 OFFSET $2",
            2,nullptr,params,nullptr,nullptr,0);

        if (PQresultStatus(res)!=PGRES_TUPLES_OK) {
            string err = PQerrorMessage(conn);
            PQclear(res);
            return crow::response(500,"Query failed: "+err);
        }

        crow::json::wvalue arr;
        int rows = PQntuples(res);
        for(int i=0;i<rows;i++) {
            arr[i]["id"]=PQgetvalue(res,i,0);
            arr[i]["fullName"]=PQgetvalue(res,i,1);
            arr[i]["phone"]=PQgetvalue(res,i,2);
            arr[i]["city"]=PQgetvalue(res,i,3);
            arr[i]["propertyType"]=PQgetvalue(res,i,4);
            arr[i]["budgetMin"]=PQgetvalue(res,i,5);
            arr[i]["budgetMax"]=PQgetvalue(res,i,6);
            arr[i]["timeline"]=PQgetvalue(res,i,7);
            arr[i]["status"]=PQgetvalue(res,i,8);
            arr[i]["updatedAt"]=PQgetvalue(res,i,9);
        }
        PQclear(res);
        return crow::response(200,arr);
    });

    // --- View Buyer with History ---
    CROW_ROUTE(app, "/buyers/<string>").methods("GET"_method)
    ([conn](const crow::request& req, const std::string& id) {
        const char* params[1] = { id.c_str() };
        PGresult* res = PQexecParams(conn,
            "SELECT id, fullName, email, phone, city, propertyType, bhk, purpose, "
            "budgetMin, budgetMax, timeline, source, status, ownerId, notes, tags, "
            "updatedAt "
            "FROM buyers WHERE id = $1",
            1, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
            PQclear(res);
            return crow::response(404, "Buyer not found");
        }

        crow::json::wvalue buyer;
        buyer["id"]          = PQgetvalue(res, 0, 0);
        buyer["fullName"]    = PQgetvalue(res, 0, 1);
        buyer["email"]       = PQgetvalue(res, 0, 2);
        buyer["phone"]       = PQgetvalue(res, 0, 3);
        buyer["city"]        = PQgetvalue(res, 0, 4);
        buyer["propertyType"]= PQgetvalue(res, 0, 5);
        buyer["bhk"]         = PQgetvalue(res, 0, 6);
        buyer["purpose"]     = PQgetvalue(res, 0, 7);
        buyer["budgetMin"]   = PQgetvalue(res, 0, 8);
        buyer["budgetMax"]   = PQgetvalue(res, 0, 9);
        buyer["timeline"]    = PQgetvalue(res, 0,10);
        buyer["source"]      = PQgetvalue(res, 0,11);
        buyer["status"]      = PQgetvalue(res, 0,12);
        buyer["ownerId"]     = PQgetvalue(res, 0,13);
        buyer["notes"]       = PQgetvalue(res, 0,14);
        buyer["tags"]        = PQgetvalue(res, 0,15);
        buyer["updatedAt"]   = PQgetvalue(res, 0,16);
        PQclear(res);

        // Fetch last 5 history
        PGresult* histRes = PQexecParams(conn,
            "SELECT field, oldValue, newValue, changedAt, changedBy "
            "FROM buyer_history WHERE buyerId = $1 "
            "ORDER BY changedAt DESC LIMIT 5",
            1, nullptr, params, nullptr, nullptr, 0);

        crow::json::wvalue historyArr;
        int rows = PQntuples(histRes);
        for (int i = 0; i < rows; i++) {
            historyArr[i]["field"]     = PQgetvalue(histRes, i, 0);
            historyArr[i]["oldValue"]  = PQgetvalue(histRes, i, 1);
            historyArr[i]["newValue"]  = PQgetvalue(histRes, i, 2);
            historyArr[i]["changedAt"] = PQgetvalue(histRes, i, 3);
            historyArr[i]["changedBy"] = PQgetvalue(histRes, i, 4);
        }
        PQclear(histRes);

        crow::json::wvalue resJson;
        resJson["buyer"]   = std::move(buyer);
        resJson["history"] = std::move(historyArr);

        return crow::response(200, resJson);
    });

    // --- Edit Buyer ---
    CROW_ROUTE(app, "/buyers/<string>").methods("PUT"_method)
    ([conn](const crow::request& req, const std::string& id) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "Invalid JSON");

        string clientUpdatedAt = body["updatedAt"].s();

        // Concurrency check
        const char* params1[1] = { id.c_str() };
        PGresult* resCheck = PQexecParams(conn,
            "SELECT fullName, phone, city, status, notes, updatedAt "
            "FROM buyers WHERE id=$1",
            1, nullptr, params1, nullptr, nullptr, 0);

        if (PQntuples(resCheck) == 0) {
            PQclear(resCheck);
            return crow::response(404, "Buyer not found");
        }

        string dbFullName = PQgetvalue(resCheck, 0, 0);
        string dbPhone    = PQgetvalue(resCheck, 0, 1);
        string dbCity     = PQgetvalue(resCheck, 0, 2);
        string dbStatus   = PQgetvalue(resCheck, 0, 3);
        string dbNotes    = PQgetvalue(resCheck, 0, 4);
        string dbUpdatedAt= PQgetvalue(resCheck, 0, 5);

        PQclear(resCheck);

        if (dbUpdatedAt != clientUpdatedAt) {
            return crow::response(409, "Record changed, please refresh");
        }

        // Get new values from request
        string fullName = body.has("fullName") ? body["fullName"].s() : dbFullName;
        string phone    = body.has("phone")    ? body["phone"].s()    : dbPhone;
        string city     = body.has("city")     ? body["city"].s()     : dbCity;
        string status   = body.has("status")   ? body["status"].s()   : dbStatus;
        string notes    = body.has("notes")    ? body["notes"].s()    : dbNotes;

        // Update buyer
        const char* params2[6] = {
            fullName.c_str(),
            phone.c_str(),
            city.c_str(),
            status.c_str(),
            notes.c_str(),
            id.c_str()
        };

        PGresult* res = PQexecParams(conn,
            "UPDATE buyers SET fullName=$1, phone=$2, city=$3, status=$4, notes=$5, updatedAt=now() "
            "WHERE id=$6 RETURNING updatedAt",
            6, nullptr, params2, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            string err = PQerrorMessage(conn);
            PQclear(res);
            return crow::response(500, "Update failed: " + err);
        }

        string newUpdatedAt = PQgetvalue(res, 0, 0);
        PQclear(res);

        // Build history inserts
        auto insertHistory = [&](const string& field, const string& oldVal, const string& newVal){
            if (oldVal != newVal) {
                const char* hparams[5] = { id.c_str(), field.c_str(), oldVal.c_str(), newVal.c_str(), "user-1" };
                PGresult* hres = PQexecParams(conn,
                    "INSERT INTO buyer_history (buyerId, field, oldValue, newValue, changedBy) "
                    "VALUES ($1,$2,$3,$4,$5)",
                    5, nullptr, hparams, nullptr, nullptr, 0);
                PQclear(hres);
            }
        };

        insertHistory("fullName", dbFullName, fullName);
        insertHistory("phone",    dbPhone,    phone);
        insertHistory("city",     dbCity,     city);
        insertHistory("status",   dbStatus,   status);
        insertHistory("notes",    dbNotes,    notes);

        // Return response
        crow::json::wvalue resJson;
        resJson["message"]   = "Buyer updated successfully";
        resJson["updatedAt"] = newUpdatedAt;
        return crow::response(200, resJson);
    });

    CROW_ROUTE(app, "/buyers/import").methods("POST"_method)
    ([conn](const crow::request& req){
        auto body = req.body; // assuming raw CSV
        std::stringstream ss(body);
        std::string line;
        int rowNum = 0;
        crow::json::wvalue errors;
        std::vector<std::vector<std::string>> validRows;

        // skip header
        getline(ss, line);

        while(getline(ss, line)) {
            rowNum++;
            std::stringstream ls(line);
            std::string cell;
            std::vector<std::string> row;
            while(getline(ls, cell, ',')) row.push_back(cell);

            if(row.size() != 14) {
                errors[rowNum] = "Incorrect number of columns";
                continue;
            }

            // Example validations
            if(row[0].empty()) { errors[rowNum] = "fullName required"; continue; }
            if(row[2].empty() || !all_of(row[2].begin(), row[2].end(), ::isdigit)) {
                errors[rowNum] = "Invalid phone"; continue;
            }

            validRows.push_back(row);
        }

        if(!validRows.empty()) {
            PQexec(conn, "BEGIN");
            for(auto& r : validRows){
                const char* params[14];
                for(int i=0;i<14;i++) params[i] = r[i].c_str();

                PGresult* res = PQexecParams(conn,
                    "INSERT INTO buyers (fullName,email,phone,city,propertyType,bhk,purpose,"
                    "budgetMin,budgetMax,timeline,source,notes,tags,status) "
                    "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14)",
                    14, nullptr, params, nullptr, nullptr, 0);

                if(PQresultStatus(res)!=PGRES_COMMAND_OK){
                    PQexec(conn,"ROLLBACK");
                    PQclear(res);
                    return crow::response(500,"Insert failed");
                }
                PQclear(res);
            }
            PQexec(conn,"COMMIT");
        }

        crow::json::wvalue resJson;
        resJson["inserted"] = validRows.size();
        resJson["errors"] = errors;
        return crow::response(200,resJson);
    });
    
    CROW_ROUTE(app,"/buyers/export").methods("GET"_method)
    ([conn](const crow::request& req){
        std::string csv = "fullName,email,phone,city,propertyType,bhk,purpose,budgetMin,budgetMax,timeline,source,notes,tags,status\n";

        PGresult* res = PQexec(conn,"SELECT fullName,email,phone,city,propertyType,bhk,purpose,"
                                "budgetMin,budgetMax,timeline,source,notes,tags,status FROM buyers");
        int rows = PQntuples(res);
        for(int i=0;i<rows;i++){
            for(int j=0;j<14;j++){
                csv += PQgetvalue(res,i,j);
                if(j<13) csv += ",";
            }
            csv += "\n";
        }
        PQclear(res);

        crow::response r;
        r.code = 200;
        r.set_header("Content-Type","text/csv");
        r.set_header("Content-Disposition","attachment; filename=buyers.csv");
        r.write(csv);
        return r;
    });


    cout<<"Server running at http://localhost:18080\n";
    app.port(18080).multithreaded().run();

    PQfinish(conn);
    return 0;
}
