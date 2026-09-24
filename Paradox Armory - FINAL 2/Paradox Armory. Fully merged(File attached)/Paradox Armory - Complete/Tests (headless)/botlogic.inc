// ---------------------------------------------------------------------
// BOT
// ---------------------------------------------------------------------
static int   botFrames    = 0;
static int   fireTimer    = 0;
static int   retreat      = 0;      // frames left of a back-off burst
static int   stall        = 0;
static float lastX        = 0.0f;
static int   shotsFired   = 0;
static int   curTarget    = -1;
static int   kiteCycle    = 0;
static float maxWx        = 0.0f;
static int   escFires     = 0;
static int   botMove=0; static bool botJump=false, botCrouch=false; static float botDist=-1; static bool botHaveTgt=false;

// How often the bot taps F. A human mashing F does far better than this;
// 12 ticks is a deliberately modest ~5 shots/second.
static const int BOT_FIRE_PERIOD = 12;

static bool solidAt(float wx, int h, int* outType) {
    for (int i = 0; i < l4ObstacleCount; i++) {
        const L4Obstacle& o = l4Obstacles[i];
        if (!o.active || !o.solid) continue;
        if (o.type == L4_OB_GATE && l4SectionCleared[o.section]) continue;
        if (!L4Rect(wx, (float)playerY, (float)playerWidth, (float)h,
                    o.x, o.y, (float)o.w, (float)o.h)) continue;
        if (outType) *outType = o.type;
        return true;
    }
    return false;
}

static bool hazardAt(float wx, int h) {
    for (int i = 0; i < l4ObstacleCount; i++) {
        const L4Obstacle& o = l4Obstacles[i];
        if (!o.active || o.solid || o.damage <= 0) continue;
        if (L4Rect(wx, (float)playerY, (float)playerWidth, (float)h,
                   o.x, o.y, (float)o.w, (float)o.h)) return true;
    }
    return false;
}

// If the player is standing in a damaging patch, which way is the short
// way out? Returns 0 when clear, -1 to step left, +1 to step right.
static int hazardEscapeDir(float wx, int h) {
    for (int i = 0; i < l4ObstacleCount; i++) {
        const L4Obstacle& o = l4Obstacles[i];
        if (!o.active || o.solid || o.damage <= 0) continue;
        if (!L4Rect(wx, (float)playerY, (float)playerWidth, (float)h,
                    o.x, o.y, (float)o.w, (float)o.h)) continue;
        float outLeft  = (wx + playerWidth) - o.x;          // distance to clear leftwards
        float outRight = (o.x + o.w) - wx;                  // distance to clear rightwards
        return (outLeft <= outRight) ? -1 : +1;
    }
    return 0;
}

static void botFrame() {
    botFrames++;
    float wx  = worldX();
    float pcx = wx + playerWidth * 0.5f;
    if (wx > maxWx) maxWx = wx;

    // ---- choose a target (sticky) -----------------------------------
    // Hysteresis matters: two enemies straddling the player at equal
    // distance would otherwise flip the bot's facing every single frame
    // and every shot would miss.
    if (curTarget >= 0) {
        const L4Enemy& t = l4Enemies[curTarget];
        if (!t.active || fabs((t.x + t.w * 0.5f) - pcx) > 820.0f) curTarget = -1;
    }
    {
        int best = -1; float bestD = 1e9f;
        for (int i = 0; i < L4_MAX_ENEMIES; i++) {
            if (!l4Enemies[i].active) continue;
            float d = (float)fabs((l4Enemies[i].x + l4Enemies[i].w * 0.5f) - pcx);
            if (d > 760.0f) continue;
            if (d < bestD) { bestD = d; best = i; }
        }
        if (curTarget < 0) curTarget = best;
        else if (best >= 0 && best != curTarget) {
            float cd = (float)fabs((l4Enemies[curTarget].x + l4Enemies[curTarget].w * 0.5f) - pcx);
            if (bestD < cd - 220.0f) curTarget = best;   // only switch for a much closer threat
        }
    }

    float dx = 0.0f; bool haveTgt = false; bool contactThreat = false;
    if (curTarget >= 0) {
        const L4Enemy& t = l4Enemies[curTarget];
        dx = (t.x + t.w * 0.5f) - pcx; haveTgt = true;
        contactThreat = (t.type != L4_RANGED);
    }
    else if (l4Boss.active) {
        float d = (l4Boss.x + l4Boss.w * 0.5f) - pcx;
        if (fabs(d) < 900.0f) { dx = d; haveTgt = true; contactThreat = true; }
    }

    int  move = 0;                 // -1 left, +1 right, 0 stand still
    bool desiredRight = true;
    bool jump = false, crouch = false;

    if (haveTgt) {
        desiredRight = (dx > 0.0f);
        float dist = (float)fabs(dx);

        // KITING. The player moves at 15px/frame, a chaser at 2.4, so the
        // right play against anything that deals contact damage is to back
        // up in short bursts and shoot in the gaps. Standing toe to toe
        // with three chasers is ~97 damage per second and is simply not
        // survivable - that is a player mistake, not a level flaw.
        // Kiting is bounded: give ground inside a ~520px arena, never walk
        // the whole level backwards, and never back into a wall.
        int away = desiredRight ? -1 : +1;
        bool roomBehind = (wx > maxWx - 520.0f) &&
                          !solidAt(wx + away * 90.0f, playerHeight, 0);

        if (contactThreat && dist < 330.0f) {
            kiteCycle++;
            if ((kiteCycle % 34) < 14 && roomBehind) move = away;   // give ground
            else                                    move = 0;      // turn and fire
        }
        else {
            kiteCycle = 0;
            if (dist > 440.0f) move = desiredRight ? +1 : -1;
            else               move = 0;           // firing lane: hold still
        }
    }
    else {
        desiredRight = true;
        move = +1;                                 // advance
        retreat = 0;
    }

    // ---- facing discipline -----------------------------------------
    // playerFacingRight only changes when A or D is pressed, so standing
    // still PRESERVES the facing. That is what lets the bot hold a firing
    // lane instead of flip-flopping and missing every shot.
    bool facingOK = (playerFacingRight == desiredRight);
    if (move == 0 && !facingOK) move = desiredRight ? +1 : -1;   // one tap to turn

    // ---- terrain in the direction of travel ------------------------
    int dirSign = (move != 0) ? move : (desiredRight ? 1 : -1);
    int standH  = playerHeight;
    int crouchH = playerHeight / 2;
    int obType  = -1;

    if (solidAt(wx, standH, &obType) && obType == L4_OB_LOWBAR) crouch = true;
    for (int step = 40; step <= 120; step += 40) {
        float probe = wx + dirSign * (float)step;
        obType = -1;
        if (!solidAt(probe, crouch ? crouchH : standH, &obType)) continue;
        if (obType == L4_OB_LOWBAR)    crouch = true;
        else if (obType != L4_OB_GATE) jump   = true;
        else                           move   = 0;   // closed gate: stop and fight
    }
    if (hazardAt(wx + dirSign * 70.0f, standH)) jump = true;

    // Standing in a spike strip costs 60 HP per second of i-frames. Get
    // out by the shortest route instead of trading shots on top of it.
    {
        int esc = hazardEscapeDir(wx, standH);
        if (esc != 0) { move = esc; jump = true; escFires++; }
    }

    // Never back into a hazard while retreating - that is how the first
    // version of this bot bled 540 HP to the same two spike strips.
    if (move != 0 && hazardAt(wx + move * 90.0f, standH)) {
        if (!hazardAt(wx - move * 90.0f, standH)) move = -move;
        else { move = 0; jump = true; }
    }

    // ---- dodge boss shockwaves -------------------------------------
    for (int i = 0; i < L4_MAX_EBULLETS; i++) {
        const L4Bullet& b = l4Bullets[i];
        if (!b.active || b.kind != 2) continue;
        if (fabs((b.x + b.w * 0.5f) - pcx) < 170.0f) jump = true;
    }

    // ---- anti-stall shuffle ----------------------------------------
    if (stall > 25) {
        int ph = (stall / 18) % 3;
        if (ph == 0)      { jump = true;   crouch = false; }
        else if (ph == 1) { crouch = true; jump = false; }
        else              { move = -dirSign; jump = true; }
    }

    botMove=move; botJump=jump; botCrouch=crouch; botDist=haveTgt?(float)fabs(dx):-1.0f; botHaveTgt=haveTgt;
    key('D', move > 0); key('A', move < 0);
    key('W', jump);     key('S', crouch);

    // ---- shoot ------------------------------------------------------
    if (--fireTimer <= 0 && (!haveTgt || facingOK)) {
        HandlePlayerKeyboardInput('f');
        fireTimer = BOT_FIRE_PERIOD;
        shotsFired++;
    }

    if (fabs(wx - lastX) < 0.6f) stall++; else stall = 0;
    lastX = wx;
}
