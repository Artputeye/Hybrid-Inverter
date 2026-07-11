#include "simulate.h"

void simulateData()
{
    int i;
    i = random(0, 100);
    if (inv.test)
    {
        // Serial.println("Test on");
        float volt = 230.3 + (float(i) / 10) + (float(i) / 100);
        float fre = 50.1 + (float(i) / 100);
        float vbus = 360 + (i / 10);
        float t = 30 + (i / 10);
        float appar = 4200 + (i / 10);
        float active = 3670 + (i / 10);
        float per = (active / 4200) * 100;
        float pvc = 1.2 + (float(i) / 100);
        float pvv = 335.7 + (float(i) / 10) + (float(i) / 100);
        float batt = 23.3 + (float(i) / 10);
        float pvpower = pvc * pvv;
        float curr = appar / volt; // I=S/V
        float pf = active / appar;

        // Monitor/////////////////////////////////////////////////
        inv.data.loadPercent = per;
        // Energy
        // Grid Power
        inv.data.ActivePower = active;
        inv.data.ApparentPower = appar;
        inv.data.outputVoltage = volt;
        inv.data.outputCurrent = curr;
        inv.data.outputFrequency = fre;
        inv.data.powerFactor = pf;
        inv.data.pvPower = pvpower;
        inv.data.pvCurrent = pvc;
        inv.data.pvVoltage = pvv;
        inv.data.busVoltage = vbus;
        inv.data.batteryVoltage = batt;
        inv.data.temp = t;

        // Status////////////////////////////////////////////////

        inv.rated.OutputRatingVoltage = 230;
        inv.rated.OutputRatingFrequency = 50;
        inv.rated.OutputRatingCurrent = 20;
        inv.rated.OutputRatingApparentPower = 1234;
        inv.rated.OutputRatingActivePower = 1221;
        inv.rated.BatteryRatingVoltage = 24.8;
        inv.rated.BatteryReChargeVoltage = 21;
        inv.rated.BatteryUnderVoltage = 22;
        inv.rated.BatteryBulkVoltage = 22;
        inv.rated.BatteryFloatVoltage = 21;
        inv.rated.MaxAC_ChargingCurrent = 5;
        inv.rated.MaxChargingCurrent = 14;

        // Home Assistant////////////////////////////////////////
        if (LoadPercent != nullptr)
        {
            LoadPercent->setValue(per);
            EnergyDaily->setValue(energy_kWh);
            GridPower->setValue(gridPower);
            ActivePower->setValue(active);
            ApparentPower->setValue(appar);
            OutputVolt->setValue(volt);
            OutputCurrent->setValue(curr);
            OutputFrequency->setValue(fre);
            PowerFactor->setValue(pf);
            pvPower->setValue(pvpower);
            pvCurrent->setValue(pvc);
            pvVoltage->setValue(pvv);
            BusVoltage->setValue(vbus);
            // BattVoltage->setValue(...); // ถ้ามีใน simulate ก็เปลี่ยนเป็น -> เช่นกันครับ
            Temp->setValue(t);
        }
        Serial.println("Voltage:" + String(volt, 2));
        Serial.println("Energy:" + String(energy_kWh, 3));
    }
}