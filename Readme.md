# 🏠 Buyer Management API

A **C++ REST API** built with **Crow** and **PostgreSQL** to manage buyer information, track changes, and support CSV import/export.

---

## 📑 Table of Contents

- [🚀 Features](#-features)
- [🛠 Tech Stack](#-tech-stack)
- [📦 Database Setup](#-database-setup)
- [⚙️ Build & Run](#-build--run)
  - [1️⃣ Install Dependencies](#1-install-dependencies)
  - [2️⃣ Compile the Project](#2-compile-the-project)
  - [3️⃣ Run the API](#3-run-the-api)
- [🔗 API Endpoints](#-api-endpoints)
  - [1️⃣ Create Buyer](#1-create-buyer)
  - [2️⃣ List Buyers](#2-list-buyers)
  - [3️⃣ View Buyer with History](#3-view-buyer-with-history)
  - [4️⃣ Edit Buyer](#4-edit-buyer)
  - [5️⃣ CSV Import / Export](#5-csv-import--export)
- [🧪 Postman Usage](#-postman-usage)
- [💻 Node.js Integration (Optional)](#-nodejs-integration-optional)
- [📄 License](#-license)

---

## 🚀 Features

- Create, view, and edit buyers  
- Track buyer history (last 5 changes)  
- Concurrency control using `updatedAt`  
- CSV import (max 200 rows) with validation  
- CSV export of current filtered list  
- Pagination support for buyer list  

---

## 🛠 Tech Stack

- **Backend:** C++ with Crow web framework  
- **Database:** PostgreSQL  
- **Database Driver:** libpq  
- **JSON Handling:** Crow's JSON module  
- **Optional:** Node.js for front-end integration  

---

## 📦 Database Setup

```sql
-- Buyers table
CREATE TABLE buyers (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    fullName TEXT,
    email TEXT,
    phone TEXT,
    city TEXT,
    propertyType TEXT,
    bhk TEXT,
    purpose TEXT,
    budgetMin INT,
    budgetMax INT,
    timeline TEXT,
    source TEXT,
    status TEXT,
    ownerId TEXT,
    notes TEXT,
    tags JSONB,
    updatedAt TIMESTAMP DEFAULT now()
);

-- Buyer history table
CREATE TABLE buyer_history (
    id SERIAL PRIMARY KEY,
    buyerId UUID REFERENCES buyers(id),
    field TEXT,
    oldValue TEXT,
    newValue TEXT,
    changedAt TIMESTAMP DEFAULT now(),
    changedBy TEXT
);
```
---
## ⚙️ Build & Run
### 1️⃣ Install Dependencies

On Windows:

- Crow (header-only library): Crow GitHub

- PostgreSQL client library (libpq)

- OpenSSL (optional, for HTTPS)

- C++ compiler (g++ from MinGW or MSYS2)

- Node.js for front-end/testing

#### Download LTS installer
```sql
curl -o node-lts.msi https://nodejs.org/dist/v20.5.0/node-v20.5.0-x64.msi
```
#### Install silently
```sql
msiexec /i node-lts.msi /quiet /norestart
```

#### Update PATH
```sql
setx PATH "%PATH%;C:\Program Files\nodejs\"
```
---

### 2️⃣ Compile the Project
```sql
g++ main.cpp -o buyer_api -IC:/path/to/crow/include -lpq -pthread -lws2_32 -lwsock32
```

---

### 3️⃣ Run the API
```sql
./buyer_api
```

#### Default server: http://localhost:18080
---

## 🔗 API Endpoints
### 1️⃣ Create Buyer

POST /buyers/new

Body (JSON):
```sql
{
  "fullName": "John Doe",
  "email": "john.doe@example.com",
  "phone": "9876543210",
  "city": "Delhi",
  "propertyType": "Apartment",
  "bhk": "2BHK",
  "purpose": "Investment",
  "budgetMin": 5000000,
  "budgetMax": 8000000,
  "timeline": "6 months",
  "source": "Website",
  "notes": "Looking for gated society",
  "tags": ["premium", "priority"]
}
```

#### Response: 201 Created with id and status.

---

### 2️⃣ List Buyers

```sql
GET /buyers?page=1
```
Returns paginated list (10 per page)

Optional page query parameter

---

### 3️⃣ View Buyer with History

GET /buyers/<uuid>

Returns buyer details + last 5 changes from buyer_history

---

### 4️⃣ Edit Buyer

PUT /buyers/<uuid>

Body (JSON):
```sql
{
  "fullName": "John Doe Updated",
  "phone": "9876543210",
  "city": "Delhi",
  "status": "Contacted",
  "notes": "Updated notes",
  "updatedAt": "2025-09-16 23:33:46"
}
```

Uses updatedAt for concurrency control

Tracks field changes in buyer_history

---

## 5️⃣ CSV Import / Export

Import: POST /buyers/import

#### Accepts CSV file with headers:
```sql
fullName,email,phone,city,propertyType,bhk,purpose,budgetMin,budgetMax,timeline,source,notes,tags,status
```
Max 200 rows

Validates rows and inserts valid rows in a transaction

Returns error table for invalid rows

Export: GET /buyers/export

Returns CSV of current filtered list

---

## 🧪 Postman Usage

Import API endpoints into Postman

Set Content-Type: application/json for POST and PUT requests

Use returned id for GET and PUT requests

Test CSV import with form-data and file type

---

## 💻 Node.js Integration (Optional)
const axios = require('axios');

async function getBuyers() {
  const res = await axios.get('http://localhost:18080/buyers?page=1');
  console.log(res.data);
}

getBuyers();

Ensure Node.js and npm are installed and available in PATH.

---

## 📄 License

MIT License – free to use and modify.