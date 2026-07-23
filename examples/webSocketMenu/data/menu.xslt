<?xml version="1.0" encoding="UTF-8"?>
<!--
  Menu renderer for OneMenu's own XmlFmt output (adapted from Rui's own
  AquaGrow/MicroTC360 menu.xslt+bootstrap.xslt). Stateless: every link
  carries its own explicit ?at=/path=, nothing relies on server-side state.
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="/">
    <html>
      <head>
        <meta charset="utf-8"/>
        <meta name="viewport" content="width=device-width, initial-scale=1"/>
        <title>OneMenu</title>
        <link rel="stylesheet" href="/style.css"/>
        <link rel="stylesheet" href="/style-dark.css" media="(prefers-color-scheme: dark)"/>
        <link rel="stylesheet" href="/style-light.css" media="(prefers-color-scheme: light)"/>
      </head>
      <body>
          <a class="masthead" href="/menu?at=/">
            <img class="logo" alt="OneMenu"
                 src="data:image/png;base64,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"/>
            <span>InternetOfPins Menu system</span>
          </a>
        <div class="panel">
          <xsl:apply-templates/>
        </div>
      </body>
    </html>
  </xsl:template>

  <!-- strip trailing "<idx>/" segment, e.g. "/3/4/" -> "/3/" -->
  <xsl:template name="parentPath">
    <xsl:param name="path"/>
    <xsl:variable name="trimmed" select="substring($path,1,string-length($path)-1)"/>
    <xsl:choose>
      <xsl:when test="contains($trimmed,'/')">
        <xsl:call-template name="lastSlashPrefix">
          <xsl:with-param name="s" select="$trimmed"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>/</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="lastSlashPrefix">
    <xsl:param name="s"/>
    <xsl:param name="acc" select="''"/>
    <xsl:choose>
      <xsl:when test="contains($s,'/')">
        <xsl:call-template name="lastSlashPrefix">
          <xsl:with-param name="s" select="substring-after($s,'/')"/>
          <xsl:with-param name="acc" select="concat($acc,substring-before($s,'/'),'/')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$acc"/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- reused for both the top-level view and a pad item's own nested menu -->
  <xsl:template match="menu">
    <xsl:apply-templates select="title"/>
    <xsl:apply-templates select="body"/>
  </xsl:template>

  <xsl:template match="view">
    <xsl:apply-templates select="menu"/>
  </xsl:template>

  <!-- title doubles as the "back" link (parent of menu/@at), except at root -->
  <xsl:template match="menu/title">
    <h3>
      <xsl:choose>
        <xsl:when test="../@at != '/'">
          <xsl:variable name="parentAt">
            <xsl:call-template name="parentPath">
              <xsl:with-param name="path" select="../@at"/>
            </xsl:call-template>
          </xsl:variable>
          <a class="title-back" href="/menu?at={$parentAt}">
            <span class="back-arrow">&#8249;</span> <xsl:apply-templates/>
          </a>
        </xsl:when>
        <xsl:otherwise><xsl:apply-templates/></xsl:otherwise>
      </xsl:choose>
    </h3>
  </xsl:template>

  <xsl:template match="body">
    <ul><xsl:apply-templates/></ul>
  </xsl:template>

  <!-- pad-mode composite field (e.g. dateField): render its nested menu/body inline -->
  <xsl:template match="item[menu]" priority="2">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <div class="pad-menu">
          <xsl:apply-templates select="menu/body"/>
        </div>
      </div>
    </li>
  </xsl:template>

  <!-- numeric field nested inside a pad-menu: plain number input, not a slider -->
  <xsl:template match="item[count(fld)=1 and lo and hi and ancestor::item[menu]]" priority="2">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <input type="number" min="{lo}" max="{hi}" value="{fld}" data-src="{@path}"
               onchange="window.location='/set?path='+this.getAttribute('data-src')+'&amp;val='+this.value+'&amp;at={/view/menu/@at}'"/>
      </div>
    </li>
  </xsl:template>

  <!-- numeric field with lo/hi: a real slider, live readout on input, /set on release -->
  <xsl:template match="item[count(fld)=1 and lo and hi]" priority="1">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <xsl:variable name="rowId">row<xsl:value-of select="translate(@path,'/','_')"/></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <input type="range" class="slider" min="{lo}" max="{hi}" value="{fld}"
               data-src="{@path}" oninput="document.getElementById('{$rowId}').textContent=this.value"
               onchange="window.location='/set?path='+this.getAttribute('data-src')+'&amp;val='+this.value+'&amp;at={/view/menu/@at}'"/>
        <output id="{$rowId}"><xsl:value-of select="fld"/></output>
        <xsl:if test="un">
          <span class="unit"><xsl:value-of select="un"/></span>
        </xsl:if>
      </div>
    </li>
  </xsl:template>

  <!-- Toggle/Select: full option list as clickable pills, radio-group style -->
  <xsl:template match="item[opt]" priority="1">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <div class="opts">
          <xsl:for-each select="opt">
            <a>
              <xsl:attribute name="class">
                <xsl:text>opt</xsl:text>
                <xsl:if test="sel=1"><xsl:text> sel</xsl:text></xsl:if>
              </xsl:attribute>
              <xsl:attribute name="href">/set?path=<xsl:value-of select="../@path"/>&amp;val=<xsl:value-of select="position()-1"/>&amp;at=<xsl:value-of select="/view/menu/@at"/></xsl:attribute>
              <xsl:value-of select="fld"/>
              <xsl:if test="un"><xsl:text> </xsl:text><xsl:value-of select="un"/></xsl:if>
            </a>
          </xsl:for-each>
        </div>
      </div>
    </li>
  </xsl:template>

  <!-- Choose's own sub-option button: sets the enclosing item, redirects to its parent -->
  <xsl:template match="item[fld and ancestor::menu[1]/@choice='1']" priority="2">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <xsl:variable name="parentAt">
      <xsl:call-template name="parentPath">
        <xsl:with-param name="path" select="ancestor::menu[1]/@at"/>
      </xsl:call-template>
    </xsl:variable>
    <li class="{$curClass}">
      <a class="opt" href="/set?path={ancestor::menu[1]/@at}&amp;val={@idx}&amp;at={$parentAt}">
        <xsl:value-of select="fld"/>
        <xsl:if test="un"><xsl:text> </xsl:text><xsl:value-of select="un"/></xsl:if>
      </a>
    </li>
  </xsl:template>

  <!-- Choose's own row in the parent list: clickable link into its own submenu -->
  <xsl:template match="item[fld and @choice='1']" priority="2">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <a class="choose-link" href="/menu?at={@path}">
        <span>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </span>
        <span class="preview">
          <xsl:value-of select="fld"/>
          <xsl:if test="un"><xsl:text> </xsl:text><xsl:value-of select="un"/></xsl:if>
        </span>
      </a>
    </li>
  </xsl:template>

  <!-- Select: a real <select><option> dropdown -->
  <xsl:template match="item[opt and @dropdown='1']" priority="2">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <select data-src="{@path}" onchange="window.location='/set?path='+this.getAttribute('data-src')+'&amp;val='+this.value+'&amp;at={/view/menu/@at}'">
          <xsl:for-each select="opt">
            <option value="{position()-1}">
              <xsl:if test="sel=1"><xsl:attribute name="selected">selected</xsl:attribute></xsl:if>
              <xsl:value-of select="fld"/>
              <xsl:if test="un"><xsl:text> </xsl:text><xsl:value-of select="un"/></xsl:if>
            </option>
          </xsl:for-each>
        </select>
      </div>
    </li>
  </xsl:template>

  <!-- field item: has a fld child, render label + a plain text input -->
  <xsl:template match="item[fld]">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <div class="field">
        <label>
          <xsl:choose>
            <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="text()"/></xsl:otherwise>
          </xsl:choose>
        </label>
        <input type="text" value="{fld}" data-src="{@path}"
               onchange="window.location='/set?path='+this.getAttribute('data-src')+'&amp;val='+this.value+'&amp;at={/view/menu/@at}'"/>
        <xsl:if test="un">
          <span class="unit"><xsl:value-of select="un"/></span>
        </xsl:if>
      </div>
    </li>
  </xsl:template>

  <!-- plain item: no fld child; @en='0' renders disabled items as non-clickable text -->
  <xsl:template match="item[not(fld)]">
    <xsl:variable name="curClass"><xsl:if test="@ncur='@'">cur</xsl:if></xsl:variable>
    <li class="{$curClass}">
      <xsl:choose>
        <xsl:when test="@en='0'">
          <span class="disabled">
            <xsl:choose>
              <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
              <xsl:otherwise><xsl:value-of select="."/></xsl:otherwise>
            </xsl:choose>
          </span>
        </xsl:when>
        <xsl:otherwise>
          <a href="/menu?at={@path}">
            <xsl:choose>
              <xsl:when test="lbl"><xsl:value-of select="lbl"/></xsl:when>
              <xsl:otherwise><xsl:value-of select="."/></xsl:otherwise>
            </xsl:choose>
          </a>
        </xsl:otherwise>
      </xsl:choose>
    </li>
  </xsl:template>

</xsl:stylesheet>
