const axios = require("axios");

const BASE_URL = "http://localhost:18080";

// --- 1) List buyers ---
async function listBuyers(page = 1) {
  try {
    const res = await axios.get(`${BASE_URL}/buyers?page=${page}`);
    console.log(res.data);
  } catch (err) {
    console.error(err.response?.data || err.message);
  }
}

// --- 2) View buyer by ID ---
async function viewBuyer(id) {
  try {
    const res = await axios.get(`${BASE_URL}/buyers/${id}`);
    console.log(res.data);
  } catch (err) {
    console.error(err.response?.data || err.message);
  }
}

// --- 3) Edit buyer ---
async function editBuyer(id, payload) {
  try {
    const res = await axios.put(`${BASE_URL}/buyers/${id}`, payload);
    console.log(res.data);
  } catch (err) {
    console.error(err.response?.data || err.message);
  }
}

// --- 4) Create a new buyer ---
async function createBuyer(payload) {
  try {
    const res = await axios.post(`${BASE_URL}/buyers/new`, payload);
    console.log(res.data);
  } catch (err) {
    console.error(err.response?.data || err.message);
  }
}

// --- Usage examples ---
(async () => {
  await listBuyers();

  await createBuyer({
    fullName: "Alice",
    phone: "9998887777",
    city: "Delhi",
    propertyType: "Apartment",
    purpose: "Investment",
    timeline: "6 months",
    source: "Website",
    budgetMin: 5000000,
    budgetMax: 8000000,
    notes: "Looking for 2BHK",
    tags: ["vip"]
  });

  // Replace with a real buyer ID from your DB
  await viewBuyer("182d6c91-f6c1-4df5-ba56-dc227baf30d2");

  await editBuyer("182d6c91-f6c1-4df5-ba56-dc227baf30d2", {
    fullName: "John Doe Updated",
    phone: "9876543210",
    city: "Delhi",
    status: "Contacted",
    notes: "Updated notes",
    updatedAt: "2025-09-16 23:33:46.897695"
  });
})();

