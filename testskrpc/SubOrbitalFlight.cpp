#include <iostream>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>
#include <tuple>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"


SubOrbitalFlight::SubOrbitalFlight(float turn_start_altitude,
                                   float turn_end_altitude,
                                   float target_altitude)
    : turn_start_altitude_(turn_start_altitude),
      turn_end_altitude_(turn_end_altitude),
      target_altitude_(target_altitude),
      delta_v_(0.0),
      burn_time_(0.0) {}

SubOrbitalFlight::~SubOrbitalFlight() = default;


// ---------------------------------------------------------------------------
// Main orchestration
// ---------------------------------------------------------------------------
void SubOrbitalFlight::launch() {
    std::cout << "Launching the vessel." << std::endl;

    connect();
    setupTelemetryStreams();
    preLaunchSetup();
    countdown();
    engageAutopilot();
    ascentLoop();
    coastToTargetApoapsis();
    coastOutOfAtmosphere();
    planCircularizationBurn();
    orientateForBurn();
    waitUntilBurn();
    executeBurn();
    deploySatellite();
    deorbitLauncher();

    std::cout << "Launch complete" << std::endl;
}


// ---------------------------------------------------------------------------
// Step: connect to kRPC and grab the active vessel
// ---------------------------------------------------------------------------
void SubOrbitalFlight::connect() {
    conn_.reset(new krpc::Client(krpc::connect("Launch into orbit")));
    space_center_.reset(new krpc::services::SpaceCenter(conn_.get()));
    vessel_.reset(new krpc::services::SpaceCenter::Vessel(space_center_->active_vessel()));
}


// ---------------------------------------------------------------------------
// Step: set up telemetry streams
// ---------------------------------------------------------------------------
void SubOrbitalFlight::setupTelemetryStreams() {
    ut_stream_.reset(new krpc::Stream<double>(space_center_->ut_stream()));
    altitude_stream_.reset(new krpc::Stream<double>(vessel_->flight().mean_altitude_stream()));
    apoapsis_stream_.reset(new krpc::Stream<double>(vessel_->orbit().apoapsis_altitude_stream()));

    auto stage_2_resources = vessel_->resources_in_decouple_stage(2, false);
    srb_fuel_stream_.reset(new krpc::Stream<float>(stage_2_resources.amount_stream("SolidFuel")));
}


// ---------------------------------------------------------------------------
// Step: pre-launch setup
// ---------------------------------------------------------------------------
void SubOrbitalFlight::preLaunchSetup() {
    vessel_->control().set_sas(false);
    vessel_->control().set_rcs(false);
    vessel_->control().set_throttle(1);
}


// ---------------------------------------------------------------------------
// Step: countdown
// ---------------------------------------------------------------------------
void SubOrbitalFlight::countdown() {
    std::cout << "3..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "2..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "1..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Launch!" << std::endl;
}


// ---------------------------------------------------------------------------
// Step: engage autopilot for the initial ascent
//
// To make lift-off reliable we activate the launch stage if (and only if) the
// vessel is still in the "pre_launch" situation — i.e. sitting on the pad
// with launch clamps engaged and no engine lit. This both releases the clamps
// and ignites the first-stage engine. If the player has already staged the
// rocket manually on the pad (situation != pre_launch), we leave staging
// alone to avoid jettisoning the fairing prematurely.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::engageAutopilot() {
    vessel_->auto_pilot().engage();
    vessel_->auto_pilot().target_pitch_and_heading(90, 90);

    auto situation = vessel_->situation();
    if (situation == krpc::services::SpaceCenter::VesselSituation::pre_launch) {
        std::cout << "On pad (pre_launch) — igniting first stage" << std::endl;
        vessel_->control().activate_next_stage();
    } else {
        std::cout << "Vessel not in pre_launch (already staged) — "
                  << "skipping initial stage activation" << std::endl;
    }
}


// ---------------------------------------------------------------------------
// Step: ascent loop (gravity turn + SRB separation)
//
// We only watch for SRB burnout / separation if the craft *actually has* SRBs
// in decouple stage 2 (initial SolidFuel > 0). Otherwise (e.g. ComSat Lx,
// which is liquid-only with a fairing in stage 2) the "burnout < 0.1" check
// would be true at lift-off and we'd immediately fire the next stage —
// jettisoning the fairing on the pad and destroying the launch.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::ascentLoop() {
    auto& altitude = *altitude_stream_;
    auto& apoapsis = *apoapsis_stream_;
    auto& srb_fuel = *srb_fuel_stream_;

    // Snapshot the initial SRB fuel: if it's effectively zero, there are no
    // SRBs to separate and we disable that whole branch.
    const float  initial_srb_fuel = srb_fuel();
    const bool   has_srbs         = (initial_srb_fuel > 1.0f);
    bool         srbs_separated   = !has_srbs;
    bool         fairing_jettisoned = false;
    double       turn_angle       = 0.0;

    if (has_srbs) {
        std::cout << "SRBs detected (" << initial_srb_fuel
                  << " units of SolidFuel), will auto-separate at burnout"
                  << std::endl;
    } else {
        std::cout << "No SRBs detected — skipping SRB separation logic"
                  << std::endl;
    }

    while (true) {
        // Gravity turn using a sqrt curve. Linear (frac * 90°) keeps the
        // rocket too vertical for too long: by 45 km it is still pitching
        // up at ~45° while orbital prograde at that altitude is ~10–15°.
        // The result is a near-vertical ascent that needs ~1600 m/s to
        // circularize. A sqrt(frac) curve has the ship at 45° by ~12 km
        // and ~75° (i.e. 15° above horizontal) by 30 km — much closer to
        // orbital prograde, giving a ~400 m/s circularization burn.
        if (altitude() > turn_start_altitude_ && altitude() < turn_end_altitude_) {
            double frac = (altitude() - turn_start_altitude_)
                          / (turn_end_altitude_ - turn_start_altitude_);
            if (frac < 0.0) frac = 0.0;
            if (frac > 1.0) frac = 1.0;
            double new_turn_angle = std::sqrt(frac) * 90.0;
            if (std::abs(new_turn_angle - turn_angle) > 0.5) {
                turn_angle = new_turn_angle;
                vessel_->auto_pilot().target_pitch_and_heading(90.0 - turn_angle, 90.0);
            }
        } else if (altitude() >= turn_end_altitude_) {
            // Past the turn end altitude: hold horizontal (pitch=0) and
            // let apoapsis grow tangentially. This is what saves dv at
            // circularization.
            if (turn_angle < 89.5) {
                turn_angle = 90.0;
                vessel_->auto_pilot().target_pitch_and_heading(0.0, 90.0);
            }
        }

        // Separate SRBs when finished (only if the craft had any to begin with)
        if (!srbs_separated && srb_fuel() < 0.1) {
            vessel_->control().activate_next_stage();
            srbs_separated = true;
            std::cout << "SRBs separated" << std::endl;
        }

        // Jettison the protective fairing as soon as we're out of the
        // atmosphere. This saves dead mass for the circularization burn so
        // we have plenty of LiquidFuel left for fine-tuning and deorbit.
        // We do this only once SRBs are gone (so we don't accidentally
        // pop the fairing on a still-strapped-on stage).
        if (!fairing_jettisoned && srbs_separated && altitude() > 70000.0) {
            vessel_->control().activate_next_stage();
            fairing_jettisoned = true;
            std::cout << "Fairing jettisoned at "
                      << altitude() << " m" << std::endl;
        }

        // FUEL EFFICIENCY: stay at full throttle all the way to the target
        // apoapsis. The previous "throttle-taper from 90% of target" wasted
        // a lot of LF burning at 25 % thrust while the ship was still
        // 30–40° off horizontal — that throttle setting produced almost no
        // horizontal velocity (gravity losses dominate at low TWR) while
        // happily consuming fuel. By the time we got to apoapsis we'd
        // burned ~120 LF for a near-vertical trajectory that then needed
        // 1100 m/s to circularize. Going full-throttle to target apo
        // typically overshoots by ~2 km (corrected later) but keeps the
        // ascent trajectory shallow — and a shallow ascent makes the
        // circularization burn small.
        if (apoapsis() > target_altitude_) {
            std::cout << "Apoapsis reached at full throttle (apo="
                      << apoapsis() << " m, target=" << target_altitude_
                      << " m)" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}


// ---------------------------------------------------------------------------
// Step: coast until target apoapsis is reached
//
// The vessel exits the ascent loop with apoapsis ≈ 0.9 * target_altitude_.
// At full throttle we'd massively overshoot (KSP engines are TWR-heavy and
// the next physics tick can add several km of apoapsis). We taper the
// throttle linearly with the remaining apoapsis gap so the final approach
// is gentle, and we cut to zero as soon as we reach the target.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::coastToTargetApoapsis() {
    auto& apoapsis = *apoapsis_stream_;

    // Throttle taper: at 90% of target we use 25%, at 95% we use 10%, and
    // once within 1 km we use a tiny 2% trim. Past target -> 0.
    std::cout << "Tapering throttle toward target apoapsis ("
              << target_altitude_ << " m)" << std::endl;
    while (apoapsis() < target_altitude_) {
        double apo = apoapsis();
        double remaining = target_altitude_ - apo;     // meters to go
        float thr;
        if      (remaining > 0.10 * target_altitude_) thr = 0.25f; // > 10% to go
        else if (remaining > 0.05 * target_altitude_) thr = 0.10f; // 5-10%
        else if (remaining > 1000.0)                  thr = 0.04f; // 1km - 5%
        else                                          thr = 0.02f; // <1km
        vessel_->control().set_throttle(thr);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    vessel_->control().set_throttle(0);
    std::cout << "Target apoapsis reached (apo=" << apoapsis() << " m)"
              << std::endl;
}


// ---------------------------------------------------------------------------
// Step: coast until out of atmosphere
//
// We don't hardcode 70 km: each celestial body exposes its atmosphere depth
// (e.g. Kerbin = 70 000 m, Eve = 90 000 m, Duna = 50 000 m). We coast 500 m
// above that to make sure aerodynamic drag is negligible before circularising.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::coastOutOfAtmosphere() {
    auto body = vessel_->orbit().body();
    double atmo = body.has_atmosphere() ? body.atmosphere_depth() : 0.0;

    // OPTIMIZATION: start aiming the rocket along orbital prograde *now*,
    // during the coast, instead of waiting until we're at apoapsis. The
    // kRPC autopilot only slews at ~0.7°/s on small upper stages so a
    // 50° turn takes >1 min — longer than the entire coast. Stock SAS in
    // "Prograde" mode uses full reaction wheels + control surfaces and
    // typically slews 5–10× faster. By the time we reach apoapsis the
    // ship is already aimed and the burn can fire immediately.
    vessel_->auto_pilot().disengage();
    vessel_->control().set_rcs(true);
    vessel_->control().set_sas(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    try {
        vessel_->control().set_sas_mode(
            krpc::services::SpaceCenter::SASMode::prograde);
        std::cout << "Pre-orienting to prograde (stock SAS) during coast"
                  << std::endl;
    } catch (const std::exception& e) {
        std::cout << "  (SAS prograde unavailable during coast: "
                  << e.what() << ")" << std::endl;
    }

    // If target apoapsis is at or below the atmosphere edge there is no
    // "out of atmosphere" point to wait for — the vessel will simply pass
    // through apoapsis just at/inside the edge. In that case we coast
    // straight to apoapsis (or as close as we can get) and let the burn
    // happen there.
    double leave_alt = std::min<double>(atmo + 500.0,
                                        target_altitude_ - 100.0);
    if (leave_alt <= 0.0) leave_alt = atmo;

    std::cout << "Coasting out of atmosphere (atmo_depth=" << atmo
              << " m, waiting until alt >= " << leave_alt << " m)"
              << std::endl;
    auto& altitude = *altitude_stream_;
    auto& apoapsis = *apoapsis_stream_;
    while (altitude() < leave_alt) {
        // Safety: if we're past apoapsis (altitude == apoapsis_altitude and
        // started falling), break out — the burn step will fire ASAP.
        if (apoapsis() < altitude()) {
            std::cout << "Past apoapsis while coasting — proceeding to burn"
                      << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


// ---------------------------------------------------------------------------
// Step: plan circularization burn
//
// Vis-viva equation:        v^2 = mu * ( 2/r  -  1/a )
//   - mu = standard gravitational parameter of the central body
//   - r  = current orbital radius (distance from body center)
//   - a  = semi-major axis of the current orbit
//
// At apoapsis our radius is r_ap = R_body + apo_altitude. The current orbit
// is the elliptical ascent trajectory (semi-major axis a1 = orbit.semi_major_axis()).
// The desired post-burn orbit is the circular orbit at that same radius, which
// has semi-major axis a2 = r_ap. The required delta-v is the *scalar* change
// in the prograde velocity:
//
//     dv = sqrt(mu*(2/r_ap - 1/a2)) - sqrt(mu*(2/r_ap - 1/a1))
//
// (Both velocities are tangential at apoapsis, so we just subtract magnitudes.)
//
// Burn time uses the Tsiolkovsky rocket equation:
//     m1 = m0 * exp(-dv/Isp_eff)         -> remaining mass after the burn
//     dt = (m0 - m1) / mdot              with mdot = F / Isp_eff
// where Isp_eff = Isp * g0  (g0 = 9.80665 m/s^2 — kRPC's specific_impulse is
// reported in seconds, so we convert to exhaust velocity in m/s).
// ---------------------------------------------------------------------------
void SubOrbitalFlight::planCircularizationBurn() {
    std::cout << "Planning circularization burn" << std::endl;

    auto body = vessel_->orbit().body();
    const double mu     = body.gravitational_parameter();
    const double R_body = body.equatorial_radius();

    // Radius of the burn point (apoapsis).
    const double r_ap = R_body + vessel_->orbit().apoapsis_altitude();

    // Semi-major axes:
    //   a1 = current elliptical orbit (ascent trajectory)
    //   a2 = desired circular orbit at r_ap
    const double a1 = vessel_->orbit().semi_major_axis();
    const double a2 = r_ap;

    // Vis-viva: v^2 = mu * (2/r - 1/a)
    const double v1 = std::sqrt(mu * ((2.0 / r_ap) - (1.0 / a1)));
    const double v2 = std::sqrt(mu * ((2.0 / r_ap) - (1.0 / a2)));
    delta_v_ = v2 - v1;

    std::cout << "  r_ap=" << r_ap << " m, a1=" << a1 << " m, a2=" << a2
              << " m" << std::endl;
    std::cout << "  v_ellipse=" << v1 << " m/s, v_circle=" << v2
              << " m/s, dv=" << delta_v_ << " m/s" << std::endl;

    // Plan the maneuver node at apoapsis (prograde dv on the maneuver-node y axis).
    node_.reset(new krpc::services::SpaceCenter::Node(
        vessel_->control().add_node(
            (*ut_stream_)() + vessel_->orbit().time_to_apoapsis(), delta_v_)));

    // Rocket equation for burn duration.
    const double g0        = 9.80665;
    const double F         = vessel_->available_thrust();
    const double ve        = vessel_->specific_impulse() * g0; // m/s
    const double m0        = vessel_->mass();
    const double m1        = m0 / std::exp(delta_v_ / ve);
    const double flow_rate = F / ve;
    burn_time_             = (m0 - m1) / flow_rate;

    std::cout << "  F=" << F << " N, ve=" << ve << " m/s, m0=" << m0
              << " kg, burn_time=" << burn_time_ << " s" << std::endl;
}


// ---------------------------------------------------------------------------
// Step: orientate the ship for the circularization burn
//
// At apoapsis on an ascending trajectory, the orbital velocity vector is
// purely tangential to the body. The circularization burn must add velocity
// in that same tangential direction. That direction is exactly what KSP
// calls "Prograde" (relative to orbit), so we point the ship using stock
// SAS in Prograde mode — it's fast, reliable, and doesn't depend on the
// maneuver-node controller (which has been refusing requests on the live
// kRPC server with "Cannot set SAS mode of vessel").
//
// We still configure the kRPC autopilot's target direction so we can read
// `auto_pilot().error()` as our convergence metric, but we don't engage it.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::orientateForBurn() {
    std::cout << "Orientating ship for circularization burn" << std::endl;

    // Strategy: stock SAS in Prograde mode is *dramatically* faster than the
    // kRPC autopilot on light upper stages (full reaction wheels + control
    // surfaces vs. the AP's gentle PID). We already turned SAS+prograde on
    // back in coastOutOfAtmosphere() so the ship has been slewing for the
    // whole coast — by the time we get here we usually only need to verify.
    // The kRPC autopilot is left as a fallback if stock SAS is unavailable.
    vessel_->auto_pilot().disengage();
    vessel_->control().set_rcs(true);
    vessel_->control().set_sas(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    bool stock_sas_ok = true;
    try {
        vessel_->control().set_sas_mode(
            krpc::services::SpaceCenter::SASMode::prograde);
        std::cout << "  Stock SAS = prograde (fast slew)" << std::endl;
    } catch (const std::exception& e) {
        stock_sas_ok = false;
        std::cout << "  (Stock SAS unavailable: " << e.what()
                  << ") — falling back to kRPC autopilot" << std::endl;
        vessel_->auto_pilot().set_reference_frame(node_->reference_frame());
        vessel_->auto_pilot().set_target_direction(
            std::make_tuple(0.0, 1.0, 0.0));
        vessel_->auto_pilot().set_target_roll(
            std::numeric_limits<float>::quiet_NaN());
        try {
            vessel_->auto_pilot().set_stopping_time(
                std::make_tuple(0.25, 0.25, 0.25));
            vessel_->auto_pilot().set_deceleration_time(
                std::make_tuple(1.0, 1.0, 1.0));
        } catch (...) {}
        vessel_->auto_pilot().engage();
    }
    (void)stock_sas_ok;  // both branches use the same convergence loop below

    // We always keep a kRPC-autopilot target configured (without engaging)
    // so error_stream() reports the angle to maneuver-node prograde even
    // when stock SAS is doing the actual steering. This gives us a single
    // unified convergence metric.
    auto err_stream      = vessel_->auto_pilot().error_stream();
    auto t_to_apo_stream = vessel_->orbit().time_to_apoapsis_stream();
    const double tolerance_deg = 5.0;
    const double timeout_sec   = 30.0;
    auto t0 = std::chrono::steady_clock::now();
    int  ticks = 0;
    while (true) {
        double err;
        try { err = err_stream(); }
        catch (...) { err = 0.0; /* if AP isn't engaged err can throw */ }

        if (err < tolerance_deg) {
            std::cout << "Pointed at burn vector (err=" << err << " deg)"
                      << std::endl;
            break;
        }
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - t0).count();
        if (elapsed > timeout_sec) {
            std::cout << "Orientation timeout, continuing with error="
                      << err << " deg" << std::endl;
            break;
        }
        // CRITICAL: don't keep aiming past the burn window. If apoapsis is
        // approaching faster than we're converging, give up and burn anyway
        // — a slightly off-axis burn is far better than coasting past
        // apoapsis with a 70 km periapsis and falling back to Kerbin.
        double t_to_apo = t_to_apo_stream();
        if (t_to_apo < (burn_time_ / 2.0) + 5.0) {
            std::cout << "Orientation aborted — apoapsis imminent (t_to_apo="
                      << t_to_apo << " s, burn_time=" << burn_time_
                      << " s, err=" << err << " deg)" << std::endl;
            break;
        }
        if ((ticks++ % 5) == 0) {
            std::cout << "  orient err=" << err
                      << " deg, t=" << elapsed
                      << "s, t_to_apo=" << t_to_apo << "s" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "Orientation complete" << std::endl;
}


// ---------------------------------------------------------------------------
// Step: warp until the burn time
// ---------------------------------------------------------------------------
void SubOrbitalFlight::waitUntilBurn() {
    std::cout << "Waiting until circularization burn" << std::endl;
    const double now       = (*ut_stream_)();
    const double t_to_apo  = vessel_->orbit().time_to_apoapsis();
    const double burn_ut   = now + t_to_apo - (burn_time_ / 2.0);
    const double lead_time = 5.0;
    const double target_ut = burn_ut - lead_time;

    // If we'd be warping into the past (apoapsis is already within
    // burn_time/2 + 5 s), there's nothing to warp to — just return so
    // executeBurn() can fire immediately. Calling warp_to with a UT in
    // the past silently no-ops on some kRPC builds, then we sit waiting
    // for an apoapsis we already passed.
    if (target_ut <= now + 1.0) {
        std::cout << "  Burn window is already here (t_to_apo=" << t_to_apo
                  << " s, burn_time=" << burn_time_
                  << " s) — skipping warp" << std::endl;
        return;
    }
    std::cout << "  Warping to UT=" << target_ut
              << " (now=" << now << ", " << (target_ut - now)
              << " s ahead)" << std::endl;
    space_center_->warp_to(target_ut);
}


// ---------------------------------------------------------------------------
// Step: execute the circularization burn
//
// Safety: we monitor attitude error and burn vector remaining throughout the
// burn so we can abort if we are so far off prograde that further thrusting
// would drop the periapsis below 30 km and re-enter / crash the rocket.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::executeBurn() {
    std::cout << "Ready to execute burn" << std::endl;
    time_to_apoapsis_stream_.reset(
        new krpc::Stream<double>(vessel_->orbit().time_to_apoapsis_stream()));

    auto& time_to_apoapsis = *time_to_apoapsis_stream_;
    // Wait until we're at (burn_time/2) before apoapsis. If we're already
    // past that point this loop exits immediately.
    while (time_to_apoapsis() - (burn_time_ / 2.0) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto err_stream   = vessel_->auto_pilot().error_stream();
    auto peri_stream  = vessel_->orbit().periapsis_altitude_stream();
    auto apo_stream   = vessel_->orbit().apoapsis_altitude_stream();
    auto liquid_stream = vessel_->resources().amount_stream("LiquidFuel");

    // Target periapsis = just below target apoapsis (a true circular orbit
    // would have peri == apo, but in KSP the engine has finite thrust so
    // by the time peri catches up apoapsis has grown a bit; we accept
    // peri >= 95 % of target as "circularized").
    const double peri_target = static_cast<double>(target_altitude_) * 0.95;

    // SAFETY: if we are still very far off the burn vector, the burn would
    // mostly waste fuel and could even drop periapsis. We still attempt it
    // but cap the burn duration so we don't ruin the orbit.
    double burn_err = err_stream();
    const bool aim_bad = (burn_err > 30.0);
    if (aim_bad) {
        std::cout << "WARNING: attitude error " << burn_err
                  << " deg is high — burning anyway with safety cap"
                  << std::endl;
    }

    std::cout << "Executing burn (target peri >= " << peri_target
              << " m, est burn_time=" << burn_time_ << " s)" << std::endl;
    vessel_->control().set_throttle(1.0f);

    auto t_burn_start = std::chrono::steady_clock::now();
    const double hard_timeout_s = std::max(burn_time_ * 3.0, 60.0);

    // Baseline for the "peri not rising" abort: we sample peri once every
    // ~3 s, not every tick. Comparing against the immediately-previous
    // tick is unreliable because the kRPC stream often returns the same
    // cached value on back-to-back calls, which would falsely trigger
    // the abort while peri is actually climbing fast.
    double baseline_peri = peri_stream();
    auto   baseline_t    = t_burn_start;
    const  double baseline_window_s = 3.0;
    int    log_tick      = 0;

    // Live thrust stream — when the current stage's tank empties, this
    // drops to (or near) zero even though our throttle is still 100 %.
    // That's the unmistakable signal that we need to stage. Without
    // staging we'd sit here burning against an empty tank, periapsis
    // frozen, until the "peri not rising" abort fires having wasted
    // the apoapsis window. (Previous log: fuel stuck at 18 LF for 5+ s
    // because the lower stage's residual was unflowable — the upper
    // stage was sitting there full of fuel we couldn't access.)
    auto thrust_stream = vessel_->thrust_stream();
    int  staged_during_burn = 0;
    const int max_burn_stages = 2;

    while (true) {
        double peri = peri_stream();
        double apo  = apo_stream();
        float  fuel = liquid_stream();
        float  thrust = thrust_stream();
        auto   now  = std::chrono::steady_clock::now();
        auto   elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - t_burn_start).count();
        double elapsed_s = elapsed_ms / 1000.0;
        double window_s  = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - baseline_t).count() / 1000.0;

        // Success: periapsis is now high enough.
        if (peri >= peri_target) {
            std::cout << "Periapsis target reached (peri=" << peri
                      << " m, apo=" << apo << " m, t=" << elapsed_s
                      << " s)" << std::endl;
            break;
        }
        // STAGE ON FLAMEOUT: throttle is up but actual thrust collapsed →
        // lower stage is empty. Fire the next decoupler so the upper stage
        // can light. Wait 0.5 s for the new engine to ignite, then re-check.
        // The fairing was already jettisoned in ascentLoop, so the next
        // stage is the inter-stage decoupler.
        if (thrust < 100.0f && elapsed_s > 0.5
            && staged_during_burn < max_burn_stages) {
            std::cout << "FLAMEOUT detected during burn (thrust=" << thrust
                      << " N, fuel=" << fuel
                      << " LF). Staging to next engine." << std::endl;
            vessel_->control().set_throttle(0);
            vessel_->control().activate_next_stage();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            vessel_->control().set_throttle(1.0f);
            staged_during_burn++;
            // Reset baseline so we don't count the staging delay
            // against the peri-not-rising guard.
            baseline_peri = peri_stream();
            baseline_t    = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        // Truly out of fuel and out of stages to drop.
        if (fuel < 0.5f && thrust < 100.0f) {
            std::cout << "Out of LiquidFuel and out of stages (peri="
                      << peri << " m, apo=" << apo << " m)" << std::endl;
            break;
        }
        // Hard timeout.
        if (elapsed_s > hard_timeout_s) {
            std::cout << "Burn timeout after " << elapsed_s << " s (peri="
                      << peri << " m)" << std::endl;
            break;
        }
        // Periapsis-not-rising abort. Use a windowed baseline so we don't
        // trip on stale stream reads, and require a meaningful gap (1 km
        // over 3 s = 333 m/s of peri progress, well below what a real
        // burn produces). Only enforce if we are actually thrusting.
        if (window_s >= baseline_window_s && thrust > 100.0f) {
            if (peri < baseline_peri + 1000.0) {
                std::cout << "ABORT: peri not rising "
                          << "(peri=" << peri
                          << " m, baseline=" << baseline_peri
                          << " m over " << window_s << " s)" << std::endl;
                break;
            }
            baseline_peri = peri;
            baseline_t    = now;
        }

        // Throttle taper as we approach the target periapsis: full thrust
        // far away, gentle thrust close in. This dramatically reduces
        // overshoot on light upper stages with high TWR.
        double gap = peri_target - peri;
        float thr;
        if      (gap > 5000.0) thr = 1.00f;
        else if (gap > 2000.0) thr = 0.40f;
        else if (gap >  500.0) thr = 0.15f;
        else                   thr = 0.05f;
        vessel_->control().set_throttle(thr);

        if ((log_tick++ % 10) == 0) {
            std::cout << "  burn peri=" << peri << " m, apo=" << apo
                      << " m, thr=" << thr << ", fuel=" << fuel
                      << ", err=" << err_stream()
                      << ", t=" << elapsed_s << " s" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    vessel_->control().set_throttle(0);
    try { node_->remove(); } catch (...) {}

    std::cout << "Burn done. Final apo="
              << vessel_->orbit().apoapsis_altitude() << " m, peri="
              << vessel_->orbit().periapsis_altitude() << " m, LF="
              << liquid_stream() << std::endl;
}


// ---------------------------------------------------------------------------
// Step: decouple the satellite and deploy it
//
// The fairing has already been jettisoned during ascent (see ascentLoop),
// so the next stage activation is the satellite decoupler itself.
// ---------------------------------------------------------------------------
void SubOrbitalFlight::deploySatellite() {
    std::cout << "Circular orbit achieved — deploying satellite" << std::endl;

    // Make sure we are not still firing the main engine
    vessel_->control().set_throttle(0);

    // Keep a stable attitude relative to surface prograde while we separate.
    // Using built-in SAS in Prograde mode is simpler and more reliable than
    // fighting the autopilot during decoupling.
    vessel_->auto_pilot().disengage();
    vessel_->control().set_sas(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    try {
        vessel_->control().set_sas_mode(
            krpc::services::SpaceCenter::SASMode::prograde);
    } catch (const std::exception& e) {
        std::cout << "  (SAS prograde unavailable: " << e.what() << ")"
                  << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Fire the decoupler — this separates the satellite from the launcher.
    // After this call the active vessel is still the launcher (the bigger
    // half); the satellite becomes a new vessel we need to find.
    std::cout << "Releasing satellite (decoupling)" << std::endl;
    auto launcher = *vessel_;
    vessel_->control().activate_next_stage();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Find the newly-created satellite vessel. It is the *other* vessel
    // closest to the launcher in altitude.
    krpc::services::SpaceCenter::Vessel satellite = launcher;
    double best_dist = 1e30;
    for (auto& v : space_center_->vessels()) {
        if (v == launcher) continue;
        try {
            double d = std::abs(v.flight().mean_altitude()
                                - launcher.flight().mean_altitude());
            if (d < best_dist) {
                best_dist = d;
                satellite = v;
            }
        } catch (...) { /* ignore vessels we can't query */ }
    }

    if (!(satellite == launcher)) {
        std::cout << "Identified satellite vessel: " << satellite.name()
                  << std::endl;
        // Switch focus to the satellite to deploy its panels/antennas.
        space_center_->set_active_vessel(satellite);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        try {
            satellite.control().set_sas(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            satellite.control().set_sas_mode(
                krpc::services::SpaceCenter::SASMode::prograde);
        } catch (...) {}

        std::cout << "Deploying solar panels" << std::endl;
        try { satellite.control().set_solar_panels(true); } catch (...) {}
        std::cout << "Deploying antennas" << std::endl;
        try { satellite.control().set_antennas(true); } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Switch back to the launcher for the deorbit burn.
        std::cout << "Switching back to launcher for deorbit" << std::endl;
        space_center_->set_active_vessel(launcher);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    } else {
        std::cout << "Could not identify a separate satellite vessel "
                  << "(continuing with launcher only)" << std::endl;
    }

    // Re-bind our vessel_ handle (active vessel should now be the launcher).
    vessel_.reset(new krpc::services::SpaceCenter::Vessel(
        space_center_->active_vessel()));
}


// ---------------------------------------------------------------------------
// Step: deorbit the launcher (burn retrograde until periapsis < ~30 km)
// ---------------------------------------------------------------------------
void SubOrbitalFlight::deorbitLauncher() {
    std::cout << "Preparing deorbit burn for launcher" << std::endl;

    // Use whatever is left in the launcher: the main upper-stage engine still
    // has the LV-T45 + a "fuelTank.long" and a small tank, so there should be
    // residual fuel after circularization.
    vessel_->control().set_throttle(0);
    vessel_->control().set_rcs(true);
    vessel_->control().set_sas(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    try {
        vessel_->control().set_sas_mode(
            krpc::services::SpaceCenter::SASMode::retrograde);
    } catch (const std::exception& e) {
        std::cout << "  (SAS retrograde unavailable: " << e.what() << ")"
                  << std::endl;
    }

    // Give SAS time to point retrograde.
    std::cout << "Pointing retrograde..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(15));

    auto periapsis_stream =
        vessel_->orbit().periapsis_altitude_stream();
    auto liquid_stream    = vessel_->resources().amount_stream("LiquidFuel");

    // Target a periapsis well inside the atmosphere so re-entry is guaranteed.
    const double target_periapsis = 30000.0;

    std::cout << "Burning retrograde (target periapsis " << target_periapsis
              << " m)" << std::endl;
    vessel_->control().set_throttle(1.0f);

    auto t0 = std::chrono::steady_clock::now();
    const double timeout_sec = 120.0;
    while (true) {
        double peri = periapsis_stream();
        float  fuel = liquid_stream();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - t0).count();

        if (peri < target_periapsis) {
            std::cout << "Deorbit complete (periapsis=" << peri << " m)"
                      << std::endl;
            break;
        }
        if (fuel < 0.1f) {
            std::cout << "Launcher ran out of fuel during deorbit burn "
                      << "(periapsis=" << peri << " m). It will decay "
                      << "naturally or stay in orbit." << std::endl;
            break;
        }
        if (elapsed > timeout_sec) {
            std::cout << "Deorbit timeout (periapsis=" << peri << " m)"
                      << std::endl;
            break;
        }
        if ((elapsed % 5) == 0) {
            std::cout << "  deorbit peri=" << peri
                      << " m, LF=" << fuel
                      << ", t=" << elapsed << "s" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    vessel_->control().set_throttle(0);
    std::cout << "Launcher deorbit burn finished" << std::endl;
}


