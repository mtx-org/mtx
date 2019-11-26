<?php
  include('dbms_inc.php');
  require_once 'securimage.php';

  $link = mysql_connect($mysql_host,$mysql_user,$mysql_password)
	or die("Could not connect");

  mysql_select_db($mysql_dbms) or die("Could not select database");

  $image = new Securimage();

  $def_mtxversion = $_POST['l_mtxversion'];
  $def_osname = $_POST['l_osname'];
  $def_osversion = $_POST['l_osversion'];
  $def_description = $_POST['l_description'];
  $def_vendorid = $_POST['l_vendorid'];
  $def_productid = $_POST['l_productid'];
  $def_revision = $_POST['l_revision'];
  $def_serialnum = $_POST['l_serialnum'];
  $def_barcodes = $_POST['l_barcodes'];
  $def_eaap = $_POST['l_eaap'];
  $def_tgdp = $_POST['l_tgdp'];
  $def_canxfer = $_POST['l_canxfer'];
  $def_transports = $_POST['l_transports'] != "" ? $_POST['l_transports'] : "0";
  $def_slots = $_POST['l_slots'] != "" ? $_POST['l_slots'] : "0";
  $def_imports = $_POST['l_imports'] != "" ? $_POST['l_imports'] : "0";
  $def_transfers = $_POST['l_transfers'] != "" ? $_POST['l_transfers'] : "0";
  $def_comments = $_POST['l_comments'];
  $def_name = $_POST['l_name'];
  $def_email = $_POST['l_email'];
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-type" content="text/html;charset=UTF-8" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta name="Content-script-type" content="text/javascript" />

    <title>MTX Compatibility List - Contribute</title>

    <style type="text/css">
      <!--
        TH { background-color: aqua; text-align: right; }
        TR { background-color: white; }
        TD { text-align: left; }
        H2 { text-align: center; }
        BODY { background-color: white; }
        A:link { color: blue; text-decoration: underline; }
        A:active { text-decoration: underline; }
        A:hover { text-decoration: underline; }
        A:visited { color: blue; text-decoration: underline; }
      -->
    </style>

    <script type="text/javascript">
      <!--
        function validateShort(field)
        {
          var value;

          if (field.value == null || field.value == "")
          {
            field.value = "0";
            return true;
          }

          value = parseInt(field.value);

          if (isNaN(value))
          {
            field.value = "0";
            return true;
          }

          if (value > 65535)
          {
            window.alert("Value must be between 0 and 65535");
            return false;
          }
          return true;
        }

        function validateNumeric(e)
        {
          var keyCode;

          if (e.which)
          {
             keyCode = e.which;
          }
          else if (e.keyCode)
          {
             keyCode = e.keyCode;
          }
          else 
          {
             return true;
          }

          return keyCode < 32 || keyCode == null || (keyCode >= 48 && keyCode <= 57);
        }

        function validateLoaderInfoText(e)
        {
          var keyCode;

          if (e.which)
          {
             keyCode = e.which;
          }
          else if (e.keyCode)
          {
             keyCode = e.keyCode;
          }
          else 
          {
             return true;
          }

          return keyCode != 39;
        }

        function validateSubmit()
        {
          var e_osversion = document.getElementById("osversion");
          var e_description = document.getElementById("description");
          var e_vendorid = document.getElementById("vendorid");
          var e_productid = document.getElementById("productid");
          var e_revision = document.getElementById("revision");
          var e_transports = document.getElementById("transports");
          var e_slots = document.getElementById("slots");
          var e_transfers = document.getElementById("transfers");
          var integerValue;

          if (e_osversion.value == "")
          {
            window.alert("You must enter the version of your operating system.  Please use the output of the command 'uname -svrp' on UNIX systems.");
            e_osversion.focus();
            return false;
          }

          if (e_description.value == "")
          {
            window.alert("You must enter a description of your loader device.");
            e_description.focus();
            return false;
          }

          if (e_vendorid.value == "" || e_productid.value == "" || e_revision.value == "")
          {
            window.alert("You must enter the Vendor ID, Product ID and Revision listed by loaderinfo.");

            if (e_vendorid.value == "")
            {
              e_vendorid.focus();
            }
            else if (e_productid.value == "")
            {
              e_productid.focus();
            }
            else if (e_revision.value == "")
            {
              e_revision.focus();
            }

            return false;
          }

          integerValue = parseInt(e_transports.value);

          if (isNaN(integerValue) || integerValue < 1)
          {
            window.alert("There must be at least one Medium Transport Element.");
            e_transports.focus();
            return false;
          }

          integerValue = parseInt(e_slots.value);

          if (isNaN(integerValue) || integerValue <= 1)
          {
            window.alert("There must be more than one Storage Element.");
            e_slots.focus();
            return false;
          }

          integerValue = parseInt(e_transfers.value);

          if (isNaN(integerValue) || integerValue < 1)
          {
            window.alert("There must be at least one Data Transfer Element.");
            e_transfers.focus();
            return false;
          }

          return true;
        }
      -->
	</script>
  </head>

  <body>
    <table style="width:100%" cellspacing="0" cellpadding="10">
      <tr valign="middle">
        <th></th>
        <th style="text-align: center">
          <h1>MTX Compatibility List<br />Contribute</h1>
        </th>
      </tr>
      <tr>
        <th style="vertical-align:top; text-align:left">
          <h2>Links:</h2>
          <p>
            <a href="http://sourceforge.net/projects/mtx">SourceForge Project</a>
          </p>
          <p>
            <a href="http://mtx.opensource-sw.net">Home Page</a>
          </p>
          <p>
            <a href="http://sourceforge.net/project/showfiles.php?group_id=4626">Downloads</a>
          </p>
          <p style="font-weight:bold; text-decoration:underline">Compatibility</p>
          <p>
            <a href="compatibility.php" style="margin-left: 1em">View</a>
            <br />
            <span style="margin-left: 1em; font-weight:bold">Submit</span>
            <br />
          </p>
          <p>
            <a href="faq.html">FAQ</a>
          </p>
        </th>
        <td rowspan="2">
          Please only submit entries that you have verified as being compatible with 
          'mtx'.  <b>Please note that your EMAIL address will *NOT* be published, and 
          will be used only in the event that I have questions about your entry.</b>
          <p />
          You will need the following information:
          <ol>
            <li>
              Your operating system type and version number (e.g. "uname -svrp"
              on FreeBSD, Linux, and Solaris)
            </li>
            <li>
              Your MTX version ( mtx --version )
            </li>
            <li>
              The result of 'loaderinfo' on your loader
            </li>
          </ol> 
          Please note that the 'barcode' output from 'loaderinfo' is not accurate for
          most loaders. Please report whether barcodes actually show up when you do
          'mtx status'. 
          <p />
          <form action="verify.php" method="post"  onsubmit="return validateSubmit()">
            <div>
              <input type="hidden" name="l_enabled" value="1"/>
              <input type="hidden" name="l_worked" value="1"/>
              <table border="1">
                <tr>
                  <th style="text-align: center" colspan="4">OS and General Info</th>
                </tr>
                <tr>
                  <!-- do a pulldown for operating system name:  -->
                  <th>Operating System</th>
                  <td>
                    <select name="l_osname">
<?php
    $query_str="select osname from hosts";
    $result=mysql_query($query_str,$link) or die("</th></tr></table>Invalid query string '$query_str'");

    while ($row=mysql_fetch_assoc($result)) {
      echo '                      <option value="'.$row['osname'].'"'.($def_osname == $row['osname'] ? ' selected="selected"' : '').'>'.$row['osname'].'</option>'."\n";
    }
?>
                    </select>
                  </td>
                  <th>MTX Version</th>
                  <td>
                    <select name="l_mtxversion">
<?php
    $query_str = "select version from versions order by `key`";
    $result = mysql_query($query_str,$link) or die("</th></tr></table>Invalid query string '$query_str'");
    $num_rows = mysql_num_rows($result);

    for ($row_no = 1; $row_no <= $num_rows; $row_no++) {
      $row = mysql_fetch_assoc($result);
      echo '                      <option value="'.$row['version'].'"'.($def_mtxversion == $row['version'] || ($def_mtxversion == '' && $row_no == $num_rows) ? ' selected="selected"' : '').'>'.$row['version'].'</option>'."\n";
    }
?>
                    </select>
                  </td>
                </tr>
                <tr>
                  <th>OS Version</th>
                  <td colspan="3">
                    <input id="osversion" name="l_osversion" value="<?php echo $def_osversion ?>" type="text" size="80" maxlength="100"/>
                  </td>
                </tr>
                <tr>
                  <th>Loader Description</th>
                  <td colspan="3">
                    <input id="description" name="l_description" value="<?php echo $def_description ?>" type="text" size="80" maxlength="100"/>
                  </td>
                </tr>
                <tr>
                  <th style="text-align: center" colspan="4">LoaderInfo Output</th>
                </tr>
                <tr>
                  <th>Vendor ID</th>
                  <td>
                    <input id="vendorid" name="l_vendorid" value="<?php echo $def_vendorid ?>" type="text" size="8" maxlength="8" onkeypress="return validateLoaderInfoText(event)"/>
                  </td>
                  <th>Product ID</th>
                  <td>
                    <input id="productid" name="l_productid" value="<?php echo $def_productid ?>" type="text" size="16" maxlength="16" onkeypress="return validateLoaderInfoText(event)"/>
                  </td>
                </tr>
                <tr>
                  <th>Revision</th>
                  <td>
                    <input id="revision" name="l_revision" value="<?php echo $def_revision ?>" type="text" size="4" maxlength="4" onkeypress="return validateLoaderInfoText(event)"/>
                  </td>
                  <th>Serial Number</th>
                  <td>
                    <input name="l_serialnum" value="<?php echo $def_serialnum ?>" type="text" size="25" maxlength="25"/>
                  </td>
                </tr>
                <tr>
                  <th>Barcode Reader</th>
                  <td>
                    <select name="l_barcodes">
                      <option value="0"<?php echo ($def_barcodes == '0' ? ' selected="selected"' : '') ?>>No</option>
                      <option value="1"<?php echo ($def_barcodes == '1' ? ' selected="selected"' : '') ?>>Yes</option>
                    </select>
                  </td>
                  <th>Element Address Assignment Page (EAAP)</th>
                  <td>
                    <select name="l_eaap">
                      <option value="0"<?php echo ($def_eaap == '0' ? ' selected="selected"' : '') ?>>No</option>
                      <option value="1"<?php echo ($def_eaap == '1' ? ' selected="selected"' : '') ?>>Yes</option>
                    </select>
                  </td>
                </tr>
                <tr>
                  <th>Transport Geometry Descriptor Page</th>
                  <td>
                    <select name="l_tgdp">
                      <option value="0"<?php echo ($def_tgdp == '0' ? ' selected="selected"' : '') ?>>No</option>
                      <option value="1"<?php echo ($def_tgdp == '1' ? ' selected="selected"' : '') ?>>Yes</option>
                    </select>
                  </td>
                  <th>Can Transfer</th>
                  <td>
                    <select name="l_canxfer">
                      <option value="0"<?php echo ($def_canxfer == '0' ? ' selected="selected"' : '') ?>>No</option>
                      <option value="1"<?php echo ($def_canxfer == '1' ? ' selected="selected"' : '') ?>>Yes</option>
                    </select>
                  </td>
                </tr>
                <tr>
                  <th>Number of Medium Transport Elements</th>
                  <td>
                    <input id="transports" name="l_transports" value="<?php echo $def_transports ?>" type="text" size="5" maxlength="5" onkeypress="return validateNumeric(event)" onchange="return validateShort(this)"/>
                  </td>
                  <th>Number of Storage Elements</th>
                  <td>
                    <input id="slots" name="l_slots" value="<?php echo $def_slots ?>" type="text" size="5" maxlength="5" onkeypress="return validateNumeric(event)" onchange="return validateShort(this)"/>
                  </td>
                </tr>
                <tr>
                  <th>Number of Import/Export Elements</th>
                  <td>
                    <input id="imports" name="l_imports" value="<?php echo $def_imports ?>" type="text" size="5" maxlength="5" onkeypress="return validateNumeric(event)" onchange="return validateShort(this)"/>
                  </td>
                  <th>Number of Data Transfer Elements</th>
                  <td>
                    <input id="transfers" name="l_transfers" value="<?php echo $def_transfers ?>" type="text" size="5" maxlength="5" onkeypress="return validateNumeric(event)" onchange="return validateShort(this)"/>
                  </td>
                </tr>
                <tr>
                  <th style="text-align: center" colspan="4">Comments</th>
                </tr>
                <tr>
                  <td colspan="4" style="text-align:center">
                    <textarea name="l_comments" cols="70" rows="4"><?php echo $def_comments ?></textarea>
                  </td>
                </tr>
                <tr>
                  <th style="text-align: center" colspan="4">Personal Data</th>
                </tr>
                <tr>
                  <th>Your Name</th>
                  <td>
                    <input name="l_name" value="<?php echo $def_name ?>" type="text" size="25" maxlength="80"/>
                  </td>
                  <th>Your EMAIL Address</th>
                  <td>
                    <input name="l_email" value="<?php echo $def_email ?>" type="text" size="25" maxlength="80"/>
                  </td>
                </tr>
                <tr>
                  <td colspan="2" style="text-align:right">
                    <img src="securimage_show.php?sid=<?php echo md5(uniqid(time())); ?>" alt="Validation Image" />
                  </td>
                  <td colspan="2" style="text-align:left">
                    Input the code displayed to the left<br />
                    <input type="text" name="l_code" />
                  </td>
                </tr>
                <tr>
                  <td style="text-align:right" colspan="2">
                    <input type="submit" name="Save" value="Save"/>
                  </td>
                  <td colspan="2">
                    <input type="button" name="Cancel" value="Cancel" onclick="window.location=&quot;compatibility.php&quot;"/>
                  </td>
                </tr>
              </table>
            </div>
          </form>
        </td>
      </tr>
      <tr>
        <th style="text-align:center; vertical-align:middle">
          <p>
            <a href="http://www.dreamhost.com/r.cgi?277748">
              <img src="dh-100x75.gif" alt="DreamHost.com Logo"
                   height="75" width="100" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://validator.w3.org/check?uri=referer" >
              <img src="valid-xhtml10.png" alt="Valid XHTML 1.0 Strict" 
                   height="31" width="88" style="border:0" />
            </a>
          </p>
          <p>
            <a href="http://sourceforge.net/projects/mtx">
              <img src="http://sflogo.sourceforge.net/sflogo.php?group_id=4626&amp;type=16"
                   width="150" height="40"
                   alt="Get MTX: Media Changer Tools at SourceForge.net. Fast, secure and Free Open Source software downloads" />
            </a>
          </p>
        </th>
      </tr>
	  <tr>
		<th />
		<td>
          <hr />
          <table style="font-size:small; width:100%">
            <tr>
              <td style="text-align:left; width:33%">
                Maintained by <a href="mailto:robertnelson@users.sourceforge.net">Robert Nelson</a>
              </td>
              <td style="text-align:center; width:34%">
<?php
				$ChangedDate = preg_replace('/.*: (.+) \(.*/', '\1', '$LastChangedDate: 2009-04-25 05:56:37 +0000 (Sat, 25 Apr 2009) $');
				echo "Date changed: $ChangedDate";
?>
              </td>
              <td style="text-align:right; width:33%">
<?php
				$ChangedBy = preg_replace('/.*: (.+) \$/', '\1', '$LastChangedBy: robertnelson $');
				echo "Changed by: $ChangedBy";
?>
              </td>
            </tr>
          </table>
		</td>
	  </tr>
    </table>
  </body>
</html>
