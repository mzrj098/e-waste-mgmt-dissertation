import paho.mqtt.client as mqtt
from flask import Flask, render_template, jsonify, request
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import json

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://root:yourpassword@localhost/semis_db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# ====================== MODELS ======================
class SensorReading(db.Model):
    __tablename__ = 'sensor_readings'
    reading_id = db.Column(db.BigInteger, primary_key=True, autoincrement=True)
    bin_id = db.Column(db.String(20), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    fill_percentage = db.Column(db.Float, nullable=False)
    temperature = db.Column(db.Float, nullable=False)
    raw_json = db.Column(db.JSON)

class Classification(db.Model):
    __tablename__ = 'classifications'
    classification_id = db.Column(db.BigInteger, primary_key=True, autoincrement=True)
    reading_id = db.Column(db.BigInteger, db.ForeignKey('sensor_readings.reading_id'))
    category = db.Column(db.String(20), nullable=False)
    reason = db.Column(db.Text)

# ====================== MQTT CALLBACK ======================
def on_message(client, userdata, msg):
    payload = json.loads(msg.payload.decode())
    bin_id = payload['bin_id']
    fill = float(payload['f'])
    temp = float(payload['t'])

    # Preprocessing (as described in thesis)
    reading = SensorReading(
        bin_id=bin_id,
        fill_percentage=fill,
        temperature=temp,
        raw_json=payload
    )
    db.session.add(reading)
    db.session.commit()

    # Rule-based classification (thesis 4.3)
    if temp > 40 and fill > 60:
        cat = 'hazardous'
        reason = 'High temperature + high fill level (fire/battery risk)'
    elif fill > 80:
        cat = 'recyclable'
        reason = 'Bin almost full – ready for collection'
    else:
        cat = 'reusable'
        reason = 'Low risk – suitable for refurbishment'

    classification = Classification(reading_id=reading.reading_id, category=cat, reason=reason)
    db.session.add(classification)
    db.session.commit()

    print(f"✅ Processed bin {bin_id} | Fill: {fill}% | Temp: {temp}°C | Class: {cat}")

# ====================== MQTT SETUP ======================
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect("localhost", 1883, 60)   # or your broker
mqtt_client.subscribe("semis/bin/#")
mqtt_client.loop_start()

# ====================== DASHBOARD ROUTES ======================
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/dashboard/citizen')
def citizen_dashboard():
    # Example gamification
    return jsonify({
        "points": 250,
        "badges": ["Eco Hero", "Green Champion"],
        "recent_events": ["+50 pts – Correct drop-off at BIN001"]
    })

@app.route('/api/dashboard/recycler')
def recycler_dashboard():
    return jsonify({
        "stock": {"Smartphone": 12, "Laptop": 5},
        "total_value_usd": 1240.50
    })

@app.route('/api/dashboard/regulator')
def regulator_dashboard():
    return jsonify({
        "total_co2_saved_kg": 124.8,
        "compliance_rate": "100%",
        "passports_issued": 45
    })

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(host='0.0.0.0', port=5000, debug=True)
