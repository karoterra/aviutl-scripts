local Bend = {}

local eps = 1e-5

-- 符号関数
local function sign(x)
  if x > 0 then return 1
  elseif x < 0 then return -1
  else return 0
  end
end

-- xをaからbの間に収める
local function clamp(x, a, b)
  if x < a then return a
  elseif x > b then return b
  else return x
  end
end

-- (v1-v0)と(v2-v0)の外積
local function cross(x0, y0, x1, y1, x2, y2)
  return (x1-x0) * (y2-y0) - (x2-x0) * (y1-y0)
end

-- ptがquadの中にあるか判定
local function ptInQuad(pt, quad)
  local a = {
    cross(quad[1],quad[2], pt[1],pt[2], quad[3],quad[4]),
    cross(quad[3],quad[4], pt[1],pt[2], quad[5],quad[6]),
    cross(quad[5],quad[6], pt[1],pt[2], quad[7],quad[8]),
    cross(quad[7],quad[8], pt[1],pt[2], quad[1],quad[2]),
  }
  return (a[1] <= eps and a[2] <= eps and a[3] <= eps and a[4] <= eps), a
end

-- 点(x,y)と直線lの距離
local function distPtLine(x, y, l)
  return math.abs(l[1]*x + l[2]*y + l[3]) / math.sqrt(l[1]^2 + l[2]^2)
end

-- 平行する直線l1とl2の距離
local function distParaLine(l1, l2)
  local x, y = 0, -l1[3] / l1[2]
  if math.abs(l1[2]) < eps then x, y = -l1[3] / l1[1], 0 end
  return distPtLine(x, y, l2)
end

-- (x1,y1)と(x2,y2)を通る直線ax+by+c=0
-- 戻り値: {a, b, c}
local function calcLine(x1, y1, x2, y2)
  if x1 > x2 then x1, y1, x2, y2 = x2, y2, x1, y1 end
  local dx, dy = x2-x1, y2-y1
  return {dy, -dx, dx*y1 - dy*x1}
end

-- 長方形srcと長方形(幅w,高さh)の交差を求める(uv座標)
local function fitCurv(src, w, h)
  local corner = {{0,0}, {w,0}, {w,h}, {0,h}}
  local index, indexN, crossVal = {}, {}, {}
  for i = 1, #corner do
    local b, cv = ptInQuad(corner[i], src)
    if b then index[#index+1] = i else indexN[#indexN+1] = i end
    crossVal[i] = cv
  end
  local dst = {}
  local l1 = calcLine(src[1], src[2], src[3], src[4])
  local l2 = calcLine(src[5], src[6], src[7], src[8])
  local dl = distParaLine(l1, l2)
  if (math.abs(l1[2])<eps and math.abs(dl-w)<eps) or (math.abs(l1[1])<eps and math.abs(dl-h)<eps) then
    dst[1] = {0,0, w,0, w,h, 0,h}
  elseif #index == 0 then
    local pos, neg = {}, {}
    for i = 1, 4 do
      local s = sign(crossVal[i][1])
      if s > 0 then pos[#pos+1] = i else neg[#neg+1] = i end
    end
    if #pos == 2 then -- 平行四辺形
      if pos[1] ~= 1 then l1, l2 = l2, l1 end
      if pos[2]-pos[1] == neg[2]-neg[1] then
        local a1, b1, a2, b2 = -l1[1]/l1[2], -l1[3]/l1[2], -l2[1]/l2[2], -l2[3]/l2[2]
        dst[1] = {0,b1, w,a1*w+b1, w,a2*w+b2, 0,b2}
      else
        local a1, b1, a2, b2 = -l1[2]/l1[1], -l1[3]/l1[1], -l2[2]/l2[1], -l2[3]/l2[1]
        dst[1] = {a1*h+b1,h, b1,0, b2,0, a2*h+b2,h}
      end
    else -- 台形
      local i1 = pos[1]
      if #pos ~= 1 then
        i1 = neg[1]
        l1, l2 = l2, l1
      end
      local x, y = corner[i1][1], corner[i1][2]
      local a1, b1, a2, b2 = -l1[1]/l1[2], -l1[3]/l1[2], -l2[1]/l2[2], -l2[3]/l2[2]
      if i1 % 2 == 0 then
        dst[1] = {
          (y-b1)/a1, y, x, a1*x+b1,
          x, a2*x+b2, (y-b2)/a2, y,
        }
      else
        dst[1] = {
          (y-b1)/a1, y, x, a1*x+b1,
          x, a2*x+b2, (y-b2)/a2, y,
        }
      end
    end
  elseif #index == 1 then
    local i1 = index[1]
    local s = {}; for i = 1, 3 do s[i] = sign(crossVal[indexN[i]][1]) end
    dst[1] = {
      corner[i1][1], corner[i1][2],
      corner[i1][1], corner[i1][2],
      corner[i1][1], corner[i1][2],
      corner[i1][1], corner[i1][2],
    }
    if s[1] == s[2] and s[1] == s[3] then -- 三角形
      local i2 = (i1+1) % 4 + 1
      local d1 = distPtLine(corner[i2][1], corner[i2][2], l1)
      local d2 = distPtLine(corner[i2][1], corner[i2][2], l2)
      if d2 < d1 then l1 = l2 end
      local a1, b1 = -l1[1]/l1[2], -l1[3]/l1[2]
      if i1 % 2 == 0 then
        dst[1][4] = a1 * dst[1][3] + b1
        dst[1][5] = (dst[1][6] - b1) / a1
      else
        dst[1][3] = (dst[1][4] - b1) / a1
        dst[1][6] = a1 * dst[1][5] + b1
      end
    else -- 五角形
      if crossVal[i1 % 4 + 1][1] > 0 then l1, l2 = l2, l1 end
      local a1, b1, a2, b2 = -l1[1]/l1[2], -l1[3]/l1[2], -l2[1]/l2[2], -l2[3]/l2[2]
      if i1 % 2 == 0 then
        dst[1][4], dst[1][5] = a2 * dst[1][3] + b2, (dst[1][6] - b1) / a1
      else
        dst[1][3], dst[1][6] = (dst[1][4] - b2) / a2, a1 * dst[1][5] + b1
      end
      local i2 = (i1+1) % 4 + 1
      dst[2] = {
        dst[1][5],dst[1][6], dst[1][3],dst[1][4],
        corner[i2][1],corner[i2][2], corner[i2][1],corner[i2][2]
      }
      local y = a2 * dst[2][5] + b2
      if 0 <= y and y <= h then
        dst[2][6], dst[2][8] = y, a1 * dst[2][7] + b1
      else
        dst[2][5], dst[2][7] = (dst[2][6] - b2) / a2, (dst[2][8] - b1) / a1
      end
    end
  elseif #index == 2 then
    local i1, i2 = index[1], index[2]
    if i1 == 1 and i2 == 4 then i1, i2 = 4, 1 end
    dst[1] = {
      corner[i1][1], corner[i1][2],
      corner[i2][1], corner[i2][2],
      corner[i2][1], corner[i2][2],
      corner[i1][1], corner[i1][2],
    }
    if i2 - i1 == 2 then
      if l1[1] * (l1[3]-l2[3]) < 0 then l1, l2 = l2, l1 end
      local a1, b1, a2, b2 = -l1[1]/l1[2], -l1[3]/l1[2], -l2[1]/l2[2], -l2[3]/l2[2]
      dst[2] = {
        corner[i2][1], corner[i2][2],
        corner[i1][1], corner[i1][2],
        corner[i1][1], corner[i1][2],
        corner[i2][1], corner[i2][2],
      }
      if i1 == 1 then
        dst[1][5], dst[1][8] = (dst[1][6] - b1) / a1, a1 * dst[1][7] + b1
        dst[2][5], dst[2][8] = (dst[2][6] - b2) / a2, a2 * dst[2][7] + b2
      else
        dst[1][6], dst[1][7] = a1 * dst[1][5] + b1, (dst[1][8] - b1) / a1
        dst[2][6], dst[2][7] = a2 * dst[2][5] + b2, (dst[2][8] - b2) / a2
      end
    else
      local d1 = distPtLine(corner[indexN[1]][1], corner[indexN[1]][2], l1)
      local d2 = distPtLine(corner[indexN[1]][1], corner[indexN[1]][2], l2)
      if d2 < d1 then l1 = l2 end
      if i1 % 2 == 0 then
        local a1, b1 = -l1[2]/l1[1], -l1[3]/l1[1]
        dst[1][5] = a1 * dst[1][6] + b1
        dst[1][7] = a1 * dst[1][8] + b1
      else
        local a1, b1 = -l1[1]/l1[2], -l1[3]/l1[2]
        dst[1][6] = a1 * dst[1][5] + b1
        dst[1][8] = a1 * dst[1][7] + b1
      end
    end
  elseif #index == 3 then
    local i1, i2, i3 = index[1], index[2], index[3]
    if i1 == 1 and i2 == 2 and i3 == 4 then i1, i2, i3 = 4, 1, 2
    elseif i1 == 1 and i2 == 3 and i3 == 4 then i1, i2, i3 = 3, 4, 1
    end
    dst[1] = {
      corner[i1][1], corner[i1][2],
      corner[i2][1], corner[i2][2],
      corner[i3][1], corner[i3][2],
      corner[i1][1], corner[i1][2],
    }
    local d1 = distPtLine(corner[indexN[1]][1], corner[indexN[1]][2], l1)
    local d2 = distPtLine(corner[indexN[1]][1], corner[indexN[1]][2], l2)
    if d2 < d1 then l1 = l2 end
    local a1, b1 = -l1[1]/l1[2], -l1[3]/l1[2]
    dst[2] = {
      corner[i1][1], corner[i1][2],
      corner[i3][1], corner[i3][2],
      corner[i3][1], corner[i3][2],
      corner[i1][1], corner[i1][2],
    }
    if i1 % 2 == 0 then
      dst[2][6] = a1 * dst[2][5] + b1
      dst[2][7] = (dst[2][8] - b1) / a1
    else
      dst[2][5] = (dst[2][6] - b1) / a1
      dst[2][8] = a1 * dst[2][7] + b1
    end
  elseif #index == 4 then
    dst[1] = {0,0, w,0, w,h, 0,h}
  end
  return dst
end

local function createEdge(anc, rad, wh, hh, d)
  local c, s = d * math.cos(rad+math.pi/2), d * math.sin(rad+math.pi/2)
  return {
    anc[1]-c+wh, anc[2]-s+hh, anc[1]+c+wh, anc[2]+s+hh,
    anc[1]+c-2*s+wh, anc[2]+s+2*c+hh, anc[1]-c-2*s+wh, anc[2]-s+2*c+hh
  }
end

-- p1, p2: xy座標
-- 戻り値: uv座標
local function createSrc(p1, p2, w, h, div)
  local d = math.sqrt(w*w + h*h)
  local rad = math.atan2(p2[2] - p1[2], p2[1] - p1[1])

  local edgeS = createEdge(p1, rad, w/2, h/2, d)
  local edgeE = createEdge(p2, rad-math.pi, w/2, h/2, d)
  local srcS = fitCurv(edgeS, w, h)
  local srcE = fitCurv(edgeE, w, h)

  local srcI = {}
  local dx1, dy1 = (edgeE[3]-edgeS[1]) / div, (edgeE[4]-edgeS[2]) / div
  local dx2, dy2 = (edgeE[1]-edgeS[3]) / div, (edgeE[2]-edgeS[4]) / div
  for i = 0, div-1 do
    local c = {
      edgeS[3] + i * dx2, edgeS[4] + i * dy2,
      edgeS[1] + i * dx1, edgeS[2] + i * dy1,
      edgeS[1] + (i+1) * dx1, edgeS[2] + (i+1) * dy1,
      edgeS[3] + (i+1) * dx2, edgeS[4] + (i+1) * dy2,
    }
    local fitted = fitCurv(c, w, h)
    for j = 1, #fitted do
      srcI[#srcI+1] = fitted[j]
    end
  end

  return srcS, srcE, srcI
end

-- src(uv)をxyにし、ancSを中心としてrad回転後、ancDに平行移動
local function transformEdge(src, ancS, ancD, rad, w, h)
  local c, s = math.cos(rad), math.sin(rad)
  local dst = {}
  for i = 1, #src, 2 do
    local x, y = src[i]-w/2-ancS[1], src[i+1]-h/2-ancS[2]
    dst[i] = x*c - y*s + ancD[1]
    dst[i+1] = x*s + y*c + ancD[2]
  end
  return dst
end

local function fixCollapse(src, dst)
  local cp = {
    cross(0,0, dst[1]-dst[7],dst[2]-dst[8], dst[3]-dst[1],dst[4]-dst[2]),
    cross(0,0, dst[3]-dst[1],dst[4]-dst[2], dst[5]-dst[3],dst[6]-dst[4]),
    cross(0,0, dst[5]-dst[3],dst[6]-dst[4], dst[7]-dst[5],dst[8]-dst[6]),
    cross(0,0, dst[7]-dst[5],dst[8]-dst[6], dst[1]-dst[7],dst[2]-dst[8]),
  }
  local idxP, idxN, idxZ = {}, {}, {}
  for i = 1, 4 do
    if cp[i] > 0 then idxP[#idxP+1] = i
    elseif cp[i] < 0 then idxN[#idxN+1] = i
    else idxZ[#idxZ+1] = i
    end
  end
  if #idxZ == 0 and #idxN == 2 then
    if idxP[1] == 1 and idxP[2] == 4 then idxP[1], idxP[2] = 4, 1 end
    if idxN[1] == 1 and idxN[2] == 4 then idxN[1], idxN[2] = 4, 1 end
    local l1 = calcLine(dst[idxP[1]*2-1],dst[idxP[1]*2], dst[idxN[2]*2-1],dst[idxN[2]*2])
    local l2 = calcLine(dst[idxP[2]*2-1],dst[idxP[2]*2], dst[idxN[1]*2-1],dst[idxN[1]*2])
    local D = -1 / (l1[1] * l2[2] - l2[1] * l1[2])
    local x, y = D * (l2[2]*l1[3] - l1[2]*l2[3]), D * (-l2[1]*l1[3] + l1[1]*l2[3])
    local rate, dx = 0, dst[idxP[1]*2-1] - dst[idxN[2]*2-1]
    if dx < eps then
      rate = (y - dst[idxN[2]*2]) / (dst[idxP[1]*2] - dst[idxN[2]*2])
    else
      rate = (x - dst[idxN[2]*2-1]) / dx
    end
    dst = {dst[idxP[1]*2-1],dst[idxP[1]*2], dst[idxP[2]*2-1],dst[idxP[2]*2], x,y, x,y}
    src = {
      src[idxP[1]*2-1],src[idxP[1]*2], src[idxP[2]*2-1],src[idxP[2]*2],
      rate*src[idxP[2]*2-1]+(1-rate)*src[idxN[1]*2-1],rate*src[idxP[2]*2]+(1-rate)*src[idxN[1]*2],
      rate*src[idxP[1]*2-1]+(1-rate)*src[idxN[2]*2-1],rate*src[idxP[1]*2]+(1-rate)*src[idxN[2]*2],
    }
  end
  return src, dst
end

-- xy座標の最小値,最大値を求める
-- range: {minX, maxX, minY, maxY}
local function calcRange(quads, range)
  for i = 1, #quads do
    for j = 1, 7, 2 do
      local x, y = quads[i][j], quads[i][j+1]
      if x < range[1] then range[1] = x
      elseif x > range[2] then range[2] = x
      end
      if y < range[3] then range[3] = y
      elseif y > range[4] then range[4] = y
      end
    end
  end
  return range
end

-- アンカー等の描画用
local function drawMarker(obj, x, y, color, size, lw)
  lw = lw or 5
  obj.load("figure", "円", 0xffffff, size)
  obj.draw(x, y)
  obj.load("figure", "円", color, size-lw)
  obj.draw(x, y)
end

-- 直線の描画
local function drawLine(obj, x1, y1, x2, y2, w, color, alpha)
  w = w or 1
  color = color or 0xffffff
  alpha = alpha or 1

  obj.load("figure", "四角形", color, 1)
  local rad = math.atan2(y2-y1, x2-x1) + math.pi / 2
  local c, s = w * math.cos(rad), w * math.sin(rad)
  obj.drawpoly(
    x1 + c, y1 + s, 0,
    x1 - c, y1 - s, 0,
    x2 - c, y2 - s, 0,
    x2 + c, y2 + s, 0,
    0,0, 1,0, 1,1, 0,1, alpha
  )
end

-- obj.drawpolyのラッパ
local function drawpoly(obj, xy, ox, oy, uv, alpha)
  alpha = alpha or 1
  obj.drawpoly(
    xy[1]-ox,xy[2]-oy,0, xy[3]-ox,xy[4]-oy,0, xy[5]-ox,xy[6]-oy,0, xy[7]-ox,xy[8]-oy,0,
    uv[1],uv[2], uv[3],uv[4], uv[5],uv[6], uv[7],uv[8], alpha
  )
end

Bend.sign = sign
Bend.clamp = clamp
Bend.cross = cross
Bend.ptInQuad = ptInQuad
Bend.distPtLine = distPtLine
Bend.distParaLine = distParaLine
Bend.calcLine = calcLine
Bend.fitCurv = fitCurv
Bend.createEdge = createEdge
Bend.createSrc = createSrc
Bend.transformEdge = transformEdge
Bend.fixCollapse = fixCollapse
Bend.calcRange = calcRange
Bend.drawMarker = drawMarker
Bend.drawLine = drawLine
Bend.drawpoly = drawpoly

return Bend
