#include "PumpCalculator.h"

#include "GaussCalculator.h"
#include "../core/Pump.h"

using namespace Z;

PairTS<Complex> PumpCalculator::calcFront(PumpParams_Waist *pump, double lambda)
{
    auto w = pump->waist()->value().toSi();
    auto z = pump->distance()->value().toSi();
    auto mi = pump->MI()->value().toSi();
    GaussCalculator gauss;
    gauss.setLock(GaussCalculator::Lock::Waist);
    gauss.setLambda(lambda);
    gauss.setW0(w.T);
    gauss.setZ(z.T);
    gauss.setM2(mi.T);
    gauss.calc();
    Complex qT(gauss.reQ(), gauss.imQ());
    gauss.setW0(w.S);
    gauss.setZ(z.S);
    gauss.setM2(mi.S);
    gauss.calc();
    Complex qS(gauss.reQ(), gauss.imQ());
    return PairTS<Complex>(qT, qS);
}

PairTS<Complex> PumpCalculator::calcFront(PumpParams_Front *pump, double lambda)
{
    auto w = pump->beamRadius()->value().toSi();
    auto R = pump->frontRadius()->value().toSi();
    auto mi = pump->MI()->value().toSi();
    GaussCalculator gauss;
    gauss.setLock(GaussCalculator::Lock::Front);
    gauss.setLambda(lambda);
    gauss.setW(w.T);
    gauss.setR(R.T);
    gauss.setM2(mi.T);
    gauss.calc();
    Complex qT(gauss.reQ(), gauss.imQ());
    gauss.setW(w.S);
    gauss.setR(R.S);
    gauss.setM2(mi.S);
    gauss.calc();
    Complex qS(gauss.reQ(), gauss.imQ());
    return PairTS<Complex>(qT, qS);
}

PairTS<RayVector> PumpCalculator::calcFront(PumpParams_RayVector* pump)
{
    auto y = pump->radius()->value().toSi();
    auto v = pump->angle()->value().toSi();
    auto z = pump->distance()->value().toSi();
    RayVector rayT(y.T + z.T * tan(v.T), v.T);
    RayVector rayS(y.S + z.S * tan(v.S), v.S);
    return PairTS<RayVector>(rayT, rayS);
}

PairTS<RayVector> PumpCalculator::calcFront(PumpParams_TwoSections* pump)
{
    auto y1 = pump->radius1()->value().toSi();
    auto y2 = pump->radius2()->value().toSi();
    auto z = pump->distance()->value().toSi();
    RayVector rayT(y2.T, atan((y2.T - y1.T) / z.T));
    RayVector rayS(y2.S, atan((y2.S - y1.S) / z.S));
    return PairTS<RayVector>(rayT, rayS);
}

PairTS<Complex> PumpCalculator::calcFront(PumpParams_Complex *pump)
{
    auto re = pump->real()->value().toSi();
    auto im = pump->imag()->value().toSi();
    Complex qT(re.T, im.T);
    Complex qS(re.S, im.S);
    return PairTS<Complex>(qT, qS);
}

PairTS<Complex> PumpCalculator::calcFront(PumpParams_InvComplex *pump)
{
    // Inverted complex ROC is measured in inverted units (1/m, 1/mm)
    // and value's unit actually means 1/unit for this case (e.g. mm means 1/mm).
    // When we want to convert inverted non-SI to inverted SI units
    // we can use fromSi() function instead as it have inverted action of toSi().
    double reT = pump->real()->value().unit()->fromSi(pump->real()->value().rawValueT());
    double imT = pump->imag()->value().unit()->fromSi(pump->imag()->value().rawValueT());
    double reS = pump->real()->value().unit()->fromSi(pump->real()->value().rawValueS());
    double imS = pump->imag()->value().unit()->fromSi(pump->imag()->value().rawValueS());
    Complex qT(reT, imT);
    Complex qS(reS, imS);
    return PairTS<Complex>(1.0/qT, 1.0/qS);
}
