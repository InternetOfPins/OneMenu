<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" encoding="UTF-8"/>

<xsl:template match="/view">
  <html>
    <head>
      <title>OneMenu</title>
      <meta http-equiv="refresh" content="5"/>
      <style>
        body { font-family: monospace; background: #111; color: #0f0; padding: 10px; }
        .m  { border: 1px solid #0f0; display: inline-block; min-width: 180px; }
        .t  { font-size: 1.2em; font-weight: bold; padding: 4px 8px; border-bottom: 1px solid #0f0; }
        .i  { padding: 3px 8px; }
        .s  { background: #0f0; color: #111; }
        .n  { margin-top: 8px; }
        .n a { color: #0f0; border: 1px solid #0f0; padding: 3px 10px;
               text-decoration: none; margin-right: 3px; }
        .n a:hover { background: #0f0; color: #111; }
      </style>
    </head>
    <body>
      <div class="m"><xsl:apply-templates/></div>
      <div class="n">
        <a href="/cmd?nav=up">&#x2191;</a>
        <a href="/cmd?nav=dn">&#x2193;</a>
        <a href="/cmd?nav=en">Enter</a>
        <a href="/cmd?nav=esc">Esc</a>
      </div>
    </body>
  </html>
</xsl:template>

<xsl:template match="menu"><xsl:apply-templates/></xsl:template>

<xsl:template match="title">
  <div class="t"><xsl:value-of select="."/></div>
</xsl:template>

<xsl:template match="body"><xsl:apply-templates/></xsl:template>

<xsl:template match="item">
  <div>
    <xsl:attribute name="class">
      <xsl:choose>
        <xsl:when test="@ncur='@'">i s</xsl:when>
        <xsl:otherwise>i</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
    <xsl:value-of select="."/>
  </div>
</xsl:template>

<xsl:template match="footer">
  <div><xsl:value-of select="."/></div>
</xsl:template>

</xsl:stylesheet>
